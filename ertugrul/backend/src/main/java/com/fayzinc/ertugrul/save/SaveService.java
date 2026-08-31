package com.fayzinc.ertugrul.save;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.integrity.SaveSigner;
import com.fayzinc.ertugrul.save.dto.SaveDownloadResponse;
import com.fayzinc.ertugrul.save.dto.SaveSlotSummary;
import com.fayzinc.ertugrul.save.dto.SaveUploadRequest;
import com.fayzinc.ertugrul.save.dto.SaveUploadResponse;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.domain.Limit;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

/**
 * Cloud-save orchestration: upload, download, list, restore, prune.
 *
 * <p><b>Tranzaksiya chegarasi — bu sinfdagi eng muhim qaror.</b> Blob object
 * store'ga <i>tranzaksiyadan tashqarida</i> yoziladi ({@link #upload} umuman
 * {@code @Transactional} emas), metadata esa qisqa, qulflangan tranzaksiyada
 * ({@link SaveCommitService}). Aks holda 8 MiB'lik tarmoq yozuvi butun davomida
 * DB qulfini ushlab turardi.
 *
 * <p>Bunday tartibning narxi — nazariy "yetim obyekt": blob yozildi, keyin
 * metadata tranzaksiyasi yiqildi. Bu <b>xavfsiz yo'nalishdagi</b> xato:
 * ortiqcha obyekt joy egallaydi va tungi tozalash uni oladi. Teskarisi —
 * metadata bor, blob yo'q — o'yinchi uchun buzilgan save degani. Shuning uchun
 * tartib qat'iy: <b>avval blob, keyin metadata</b>.
 */
@Service
public class SaveService {

    private static final Logger log = LoggerFactory.getLogger(SaveService.class);

    /** Above this size the client is handed a presigned URL instead of inline base64. */
    private static final int INLINE_RESPONSE_LIMIT_BYTES = 512 * 1024;

    private final SaveRepository saveRepository;
    private final SaveVersionRepository versionRepository;
    private final SaveCommitService commitService;
    private final SaveBlobStore blobStore;
    private final SaveSigner saveSigner;
    private final ErtugrulProperties props;

    private final Counter uploadsAccepted;
    private final Counter uploadsStale;
    private final Counter uploadsConflicted;

    public SaveService(SaveRepository saveRepository,
                       SaveVersionRepository versionRepository,
                       SaveCommitService commitService,
                       SaveBlobStore blobStore,
                       SaveSigner saveSigner,
                       ErtugrulProperties props,
                       MeterRegistry meterRegistry) {
        this.saveRepository = saveRepository;
        this.versionRepository = versionRepository;
        this.commitService = commitService;
        this.blobStore = blobStore;
        this.saveSigner = saveSigner;
        this.props = props;

        this.uploadsAccepted = Counter.builder("ertugrul.save.upload")
                .tag("outcome", "accepted").register(meterRegistry);
        this.uploadsStale = Counter.builder("ertugrul.save.upload")
                .tag("outcome", "stale").register(meterRegistry);
        // A rising conflict rate means devices are diverging more than expected
        // and is the first symptom of a broken sync flow.
        this.uploadsConflicted = Counter.builder("ertugrul.save.upload")
                .tag("outcome", "conflict").register(meterRegistry);
    }

    /**
     * Save blob'ni yuklaydi va konfliktni hal qiladi.
     *
     * <p>Bosqichlar:
     * <ol>
     *   <li>Slot raqami, blob hajmi, sha256 va klient soati tekshiriladi;</li>
     *   <li>Bir xil kontent allaqachon boshda tursa — takroriy urinish deb
     *       hisoblanadi va hech narsa yozilmaydi;</li>
     *   <li>Blob object store'ga yoziladi (<b>tranzaksiyadan tashqarida</b>);</li>
     *   <li>{@link SaveCommitService} qisqa tranzaksiyada slotni qulflab,
     *       vector clock'ni solishtiradi va metadata'ni yozadi.</li>
     * </ol>
     *
     * <p>Konflikt yuz bersa mag'lub blob <b>o'chirilmaydi</b> — u saqlanadi va
     * o'yinchiga tiklash taklif qilinadi.
     *
     * @param playerId  the owning player
     * @param slotIndex 0 (autosave) or 1..8
     * @param request   blob, clock, and readable summary
     * @return what the server did and the clock the client must carry forward
     * @throws ErtugrulException on an invalid slot, oversized blob, hash
     *         mismatch, implausible client clock, or a stale upload
     */
    public SaveUploadResponse upload(UUID playerId, int slotIndex, SaveUploadRequest request) {

        validateSlot(slotIndex);

        byte[] blob = decodeBlob(request.blobBase64());
        validateBlobSize(blob);
        validateClientClock(request.clientSavedAt());

        if (!saveSigner.verifyContentHash(blob, request.sha256())) {
            throw new ErtugrulException(ErtugrulException.Code.SAVE_CORRUPT,
                    "Blob content does not match the declared sha256");
        }

        // Soft anti-cheat signal; never blocks the upload. See SaveSigner javadoc.
        Boolean clientHmacValid = saveSigner.verifyClientSignature(blob, request.clientHmac());

        SaveSlot slot = commitService.findOrCreateSlot(playerId, slotIndex);

        if (isDuplicateOfHead(slot, request.sha256())) {
            log.debug("Duplicate save upload ignored: player={} slot={} sha={}",
                    playerId, slotIndex, request.sha256());
            return SaveUploadResponse.accepted(
                    ConflictResolver.Outcome.ACCEPT_IDENTICAL,
                    slot.getHeadVersion(), slot.getVectorClock(),
                    "identical blob already stored");
        }

        // Reserve the version number, then write the blob with NO transaction open.
        long nextVersion = versionRepository.findMaxVersion(slot.getId()) + 1;
        String objectKey = blobStore.objectKey(playerId, slotIndex, nextVersion);
        String serverHmac = saveSigner.signBlobIdentity(
                playerId, slotIndex, nextVersion, request.sha256());

        blobStore.put(objectKey, blob, request.sha256(), serverHmac);

        try {
            SaveUploadResponse response = commitService.commit(
                    playerId, slotIndex, request, nextVersion,
                    objectKey, blob.length, serverHmac, clientHmacValid);

            if (response.outcome().isConflict()) {
                uploadsConflicted.increment();
            } else {
                uploadsAccepted.increment();
            }
            return response;

        } catch (ErtugrulException e) {
            if (e.code() == ErtugrulException.Code.SAVE_STALE) {
                uploadsStale.increment();
            }
            throw e;
        }
    }

    /**
     * Slotdagi joriy save'ni qaytaradi.
     *
     * <p>Server imzosi tekshiriladi: mos kelmasa klientga buzuq ma'lumot
     * berilmaydi — bu object store buzilgani yoki blob almashtirilgani
     * belgisidir.
     *
     * @param playerId  owning player
     * @param slotIndex which slot
     * @param inline    whether the caller wants the payload inline
     * @return the save, inline for small blobs or as a presigned URL for large ones
     */
    @Transactional(readOnly = true)
    public SaveDownloadResponse download(UUID playerId, int slotIndex, boolean inline) {
        validateSlot(slotIndex);

        SaveSlot slot = saveRepository.findByPlayerIdAndSlotIndex(playerId, (short) slotIndex)
                .orElseThrow(() -> ErtugrulException.saveNotFound(slotIndex));

        if (slot.isEmpty()) {
            throw ErtugrulException.saveNotFound(slotIndex);
        }

        SaveVersion version = versionRepository
                .findBySlotIdAndVersion(slot.getId(), slot.getHeadVersion())
                .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.SAVE_NOT_FOUND,
                        "Head version metadata is missing"));

        if (!saveSigner.verifyBlobIdentity(playerId, slotIndex, version.getVersion(),
                version.getSha256(), version.getServerHmac())) {
            throw new ErtugrulException(ErtugrulException.Code.SAVE_CORRUPT,
                    "Stored save failed integrity verification");
        }

        boolean sendInline = inline && version.getSizeBytes() <= INLINE_RESPONSE_LIMIT_BYTES;

        String blobBase64 = null;
        String downloadUrl = null;

        if (sendInline) {
            byte[] blob = blobStore.get(version.getObjectKey());
            // Re-check the content itself, not just the metadata: this catches a
            // silently corrupted object rather than a swapped one.
            if (!saveSigner.verifyContentHash(blob, version.getSha256())) {
                throw new ErtugrulException(ErtugrulException.Code.SAVE_CORRUPT,
                        "Stored blob does not match its recorded hash");
            }
            blobBase64 = Base64.getEncoder().encodeToString(blob);
        } else {
            downloadUrl = blobStore.presignedDownloadUrl(version.getObjectKey()).toString();
        }

        return new SaveDownloadResponse(
                slotIndex,
                version.getVersion(),
                version.getVectorClock(),
                blobBase64,
                downloadUrl,
                version.getSha256(),
                version.getServerHmac(),
                version.getSizeBytes(),
                new SaveDownloadResponse.WorldStateSummaryView(
                        slot.getEpisodeId(), slot.getSeasonId(), slot.getPlaytimeSeconds(),
                        slot.getHandIntegrity(), slot.getMaxIntegrity(),
                        slot.getDifficultyTier(), slot.getSchemaVersion()),
                version.getClientSavedAt());
    }

    /**
     * Barcha slotlarni "Davom etish" ekrani uchun qaytaradi.
     *
     * <p>Bo'sh slotlar ham qaytariladi — klient doim 9 ta qatorni ko'rsatadi va
     * qaysi biri bo'shligini o'zi hisoblamasligi kerak. Bu chaqiruv S3'ga
     * umuman bormaydi: barcha maydonlar Postgres'dagi xulosadan olinadi.
     */
    @Transactional(readOnly = true)
    public List<SaveSlotSummary> listSlots(UUID playerId) {
        List<SaveSlot> stored = saveRepository.findByPlayerIdOrderBySlotIndexAsc(playerId);

        List<SaveSlotSummary> result = new ArrayList<>(props.save().totalSlots());
        for (int index = 0; index <= props.save().maxManualSlots(); index++) {
            final int slotIndex = index;
            result.add(stored.stream()
                    .filter(s -> s.getSlotIndex() == slotIndex)
                    .findFirst()
                    .map(SaveSlotSummary::from)
                    .orElseGet(() -> SaveSlotSummary.emptySlot(slotIndex)));
        }
        return result;
    }

    /**
     * Konfliktda saqlanib qolgan save'ni tiklaydi.
     *
     * <p>O'yinchi "boshqa qurilmadagi saqlashni tiklash" ni tanlaganda
     * chaqiriladi. Joriy bosh <b>o'chirilmaydi</b> — u o'z navbatida mag'lub
     * sifatida saqlanadi, ya'ni o'yinchi fikridan qaytsa yana orqaga qaytishi
     * mumkin.
     *
     * @param playerId  owning player
     * @param slotIndex slot to restore in
     * @return the slot after restoration
     * @throws ErtugrulException {@code SAVE_NOT_FOUND} when there is nothing to restore
     */
    @Transactional
    public SaveSlotSummary restoreConflictCopy(UUID playerId, int slotIndex) {
        validateSlot(slotIndex);

        SaveSlot slot = saveRepository.findForUpdate(playerId, (short) slotIndex)
                .orElseThrow(() -> ErtugrulException.saveNotFound(slotIndex));

        SaveVersion loser = versionRepository.findLatestConflictLoser(slot.getId())
                .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.SAVE_NOT_FOUND,
                        "No conflict copy to restore in slot " + slotIndex));

        SaveVersion currentHead = versionRepository
                .findBySlotIdAndVersion(slot.getId(), slot.getHeadVersion())
                .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.SAVE_NOT_FOUND,
                        "Head version metadata is missing"));

        // Swap: the retained copy becomes head, the old head becomes retained.
        currentHead.loseTo(loser);
        loser.setResolution(SaveVersion.Resolution.LWW_WINNER);

        VectorClock merged = loser.clock().merge(currentHead.clock());
        // Summary stays as-is: the stored blob's own summary is not re-parsed
        // here, and inventing one from the loser's metadata would be worse than
        // leaving the last known values until the client's next upload.
        slot.advanceHead(loser.getVersion(), merged, null);
        slot.markConflict(true);

        log.info("Restored conflict copy for player={} slot={} version={}",
                playerId, slotIndex, loser.getVersion());

        return SaveSlotSummary.from(slot);
    }

    /** Clears the conflict flag once the player has chosen and dismissed the prompt. */
    @Transactional
    public void acknowledgeConflict(UUID playerId, int slotIndex) {
        validateSlot(slotIndex);
        saveRepository.findByPlayerIdAndSlotIndex(playerId, (short) slotIndex)
                .ifPresent(slot -> slot.markConflict(false));
    }

    /**
     * Eski versiyalarni tozalaydi — tungi job chaqiradi.
     *
     * <p>Konfliktda saqlangan nusxalar <b>hech qachon</b> tozalanmaydi: ular
     * o'yinchi uchun yagona qutqaruv yo'li.
     *
     * @param slotId slot to prune
     * @return number of versions removed
     */
    @Transactional
    public int pruneOldVersions(UUID slotId) {
        long head = versionRepository.findMaxVersion(slotId);
        long keepAbove = head - props.save().retainedVersionsPerSlot();
        if (keepAbove <= 0) {
            return 0;
        }

        List<SaveVersion> prunable = versionRepository.findPrunable(slotId, keepAbove);
        if (prunable.isEmpty()) {
            return 0;
        }

        List<String> keys = prunable.stream().map(SaveVersion::getObjectKey).toList();
        blobStore.deleteAll(keys);
        versionRepository.deleteAll(prunable);

        log.debug("Pruned {} save versions from slot={}", prunable.size(), slotId);
        return prunable.size();
    }

    /** Recent version history for a slot — support tooling and the restore UI. */
    @Transactional(readOnly = true)
    public List<SaveVersion> versionHistory(UUID playerId, int slotIndex, int limit) {
        SaveSlot slot = saveRepository.findByPlayerIdAndSlotIndex(playerId, (short) slotIndex)
                .orElseThrow(() -> ErtugrulException.saveNotFound(slotIndex));
        return versionRepository.findBySlotIdOrderByVersionDesc(slot.getId(), Limit.of(limit));
    }

    // ── internals ───────────────────────────────────────────────────────────

    /** True when the slot's current head already holds exactly these bytes. */
    private boolean isDuplicateOfHead(SaveSlot slot, String sha256) {
        if (slot.isEmpty()) {
            return false;
        }
        Optional<SaveVersion> match =
                versionRepository.findFirstBySlotIdAndSha256OrderByVersionDesc(slot.getId(), sha256);
        return match.isPresent() && match.get().getVersion() == slot.getHeadVersion();
    }

    private void validateSlot(int slotIndex) {
        if (!props.save().isValidSlot(slotIndex)) {
            throw ErtugrulException.invalidSlot(slotIndex, props.save().maxManualSlots());
        }
    }

    private void validateBlobSize(byte[] blob) {
        if (blob.length == 0) {
            throw new ErtugrulException(ErtugrulException.Code.SAVE_CORRUPT, "Blob is empty");
        }
        if (blob.length > props.save().maxBlobBytes()) {
            throw new ErtugrulException(ErtugrulException.Code.SAVE_TOO_LARGE,
                    "Blob is %d bytes, limit is %d".formatted(blob.length, props.save().maxBlobBytes()));
        }
    }

    /**
     * Klient soatini tekshiradi.
     *
     * <p>Kelajakka ketgan soat LWW ni buzadi: noto'g'ri sozlangan qurilma har
     * doim "g'olib" bo'lib qolardi. O'tmishga ketgani zararsiz — u shunchaki
     * konfliktda yutqazadi, shuning uchun tekshirilmaydi.
     */
    private void validateClientClock(Instant clientSavedAt) {
        Duration skew = Duration.between(Instant.now(), clientSavedAt);
        if (skew.compareTo(props.save().maxClockSkew()) > 0) {
            throw new ErtugrulException(ErtugrulException.Code.SAVE_CLOCK_SKEW,
                    "clientSavedAt is %d seconds in the future".formatted(skew.toSeconds()));
        }
    }

    private byte[] decodeBlob(String base64) {
        try {
            return Base64.getDecoder().decode(base64);
        } catch (IllegalArgumentException e) {
            throw new ErtugrulException(ErtugrulException.Code.SAVE_CORRUPT,
                    "blobBase64 is not valid base64", e);
        }
    }
}
