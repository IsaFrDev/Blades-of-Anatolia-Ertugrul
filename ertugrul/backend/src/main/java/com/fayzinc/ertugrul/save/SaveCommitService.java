package com.fayzinc.ertugrul.save;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.identity.PlayerRepository;
import com.fayzinc.ertugrul.integrity.SaveSigner;
import com.fayzinc.ertugrul.save.dto.SaveUploadRequest;
import com.fayzinc.ertugrul.save.dto.SaveUploadResponse;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.Optional;
import java.util.UUID;

/**
 * The transactional half of the save upload path.
 *
 * <p><b>Nega alohida bean.</b> {@link SaveService#upload} tranzaksiyasiz
 * bo'lishi <i>shart</i>: u 8 MiB blob'ni object store'ga yozadi, va bu yozuv
 * DB tranzaksiyasi ichida bo'lsa — qulf tarmoq tezligiga bog'lanib qoladi va
 * bir nechta parallel saqlash butun connection pool'ni to'sib qo'yadi.
 *
 * <p>Ammo metadata yozuvi <i>albatta</i> tranzaksiyada, qulflangan slot ustida
 * bo'lishi kerak. Agar bu metodlar {@code SaveService} ichida qolganida, Spring
 * proxy'si chetlab o'tilardi (self-invocation) va {@code @Transactional}
 * <b>umuman ishlamasdi</b> — qulf ham, atomiklik ham yo'q. Shuning uchun ular
 * shu yerda, alohida bean'da.
 */
@Service
public class SaveCommitService {

    private static final Logger log = LoggerFactory.getLogger(SaveCommitService.class);

    private final SaveRepository saveRepository;
    private final SaveVersionRepository versionRepository;
    private final PlayerRepository playerRepository;
    private final ConflictResolver conflictResolver;

    public SaveCommitService(SaveRepository saveRepository,
                             SaveVersionRepository versionRepository,
                             PlayerRepository playerRepository,
                             ConflictResolver conflictResolver) {
        this.saveRepository = saveRepository;
        this.versionRepository = versionRepository;
        this.playerRepository = playerRepository;
        this.conflictResolver = conflictResolver;
    }

    /**
     * Slotni topadi yoki yaratadi.
     *
     * <p>Poyga holati {@code uq_slot_player_index} bilan tutiladi: ikkinchi
     * so'rov cheklovni buzadi va biz shunchaki qayta o'qiymiz.
     *
     * @param playerId  owning player
     * @param slotIndex 0..8
     * @return the existing or newly created slot
     */
    @Transactional
    public SaveSlot findOrCreateSlot(UUID playerId, int slotIndex) {
        Optional<SaveSlot> existing =
                saveRepository.findByPlayerIdAndSlotIndex(playerId, (short) slotIndex);
        if (existing.isPresent()) {
            return existing.get();
        }

        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));

        try {
            return saveRepository.saveAndFlush(new SaveSlot(player, slotIndex));
        } catch (DataIntegrityViolationException race) {
            return saveRepository.findByPlayerIdAndSlotIndex(playerId, (short) slotIndex)
                    .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.INTERNAL,
                            "Slot vanished after unique-constraint violation", race));
        }
    }

    /**
     * Konfliktni hal qiladi va metadata'ni yozadi — qisqa, qulflangan
     * tranzaksiya.
     *
     * <p>Bu yerda tarmoq I/O yo'q: blob allaqachon object store'da. Shuning
     * uchun slot qulfi bir necha millisekundgina ushlab turiladi.
     *
     * @param playerId        owning player
     * @param slotIndex       slot being written
     * @param request         the original upload
     * @param version         version number already reserved and written to the store
     * @param objectKey       where the blob was written
     * @param sizeBytes       blob size
     * @param serverHmac      server signature over the blob identity
     * @param clientHmacValid soft anti-cheat result; {@code null} when unsigned
     * @return what the server decided
     * @throws ErtugrulException {@code SAVE_STALE} when the client is behind
     */
    @Transactional
    public SaveUploadResponse commit(UUID playerId, int slotIndex,
                                     SaveUploadRequest request, long version,
                                     String objectKey, int sizeBytes,
                                     String serverHmac, Boolean clientHmacValid) {

        SaveSlot slot = saveRepository.findForUpdate(playerId, (short) slotIndex)
                .orElseThrow(() -> ErtugrulException.saveNotFound(slotIndex));

        // The client sends the clock it had BEFORE this write; the server owns
        // the increment so a client cannot inflate its own counter.
        VectorClock incoming = VectorClock.of(request.vectorClock()).increment(request.deviceId());

        Optional<SaveVersion> headVersion = slot.isEmpty()
                ? Optional.empty()
                : versionRepository.findBySlotIdAndVersion(slot.getId(), slot.getHeadVersion());

        ConflictResolver.Decision decision = conflictResolver.resolve(
                incoming,
                slot.clock(),
                request.clientSavedAt(),
                headVersion.map(SaveVersion::getClientSavedAt).orElse(Instant.EPOCH),
                ConflictResolver.ProgressScore.of(
                        request.summary().episodeNumber(), request.summary().playtimeSeconds()),
                ConflictResolver.ProgressScore.of(
                        episodeNumberOf(slot.getEpisodeId()), slot.getPlaytimeSeconds()));

        if (decision.outcome() == ConflictResolver.Outcome.REJECT_STALE) {
            // Leaves an orphan blob behind; the nightly sweep collects it. Cheaper
            // than blocking this response on a delete round trip.
            throw new ErtugrulException(ErtugrulException.Code.SAVE_STALE,
                    "Upload is behind the server head; pull before saving again");
        }

        if (decision.outcome() == ConflictResolver.Outcome.ACCEPT_IDENTICAL) {
            // Practically unreachable: the incoming clock was just incremented,
            // so it cannot equal the head. Handled explicitly anyway, because
            // silently falling through to the conflict branch would retain a
            // pointless "conflict copy" and prompt the player for nothing.
            log.debug("Identical clock on upload for player={} slot={}; no-op", playerId, slotIndex);
            return SaveUploadResponse.accepted(decision.outcome(), slot.getHeadVersion(),
                    slot.getVectorClock(), decision.reason());
        }

        SaveVersion stored = new SaveVersion(slot, version, objectKey, sizeBytes,
                request.sha256(), serverHmac, decision.mergedClock(),
                request.deviceId(), request.clientSavedAt());
        stored.setClientHmacValid(clientHmacValid);

        recordSuspicionIfNeeded(playerId, clientHmacValid);

        if (decision.outcome().becomesHead()) {
            headVersion.ifPresent(previous -> {
                if (decision.outcome().isConflict()) {
                    previous.loseTo(stored);   // retained, never deleted
                } else {
                    previous.supersede();
                }
            });

            stored.setResolution(switch (decision.outcome()) {
                case ACCEPT_INITIAL -> SaveVersion.Resolution.INITIAL;
                case ACCEPT_FAST_FORWARD -> SaveVersion.Resolution.FAST_FORWARD;
                case CONFLICT_INCOMING_WINS -> SaveVersion.Resolution.LWW_WINNER;
                default -> throw new IllegalStateException(
                        "Unexpected head-becoming outcome: " + decision.outcome());
            });

            versionRepository.save(stored);
            slot.advanceHead(version, decision.mergedClock(), request.summary());
            slot.markConflict(decision.outcome().isConflict());

        } else {
            // CONFLICT_HEAD_WINS: keep the upload as a restorable conflict copy.
            headVersion.ifPresent(stored::loseTo);
            versionRepository.save(stored);
            slot.markConflict(true);
        }

        if (decision.outcome().isConflict()) {
            log.info("Save conflict on player={} slot={}: {} ({})",
                    playerId, slotIndex, decision.outcome(), decision.reason());
            return SaveUploadResponse.conflicted(
                    decision.outcome(), slot.getHeadVersion(), slot.getVectorClock(),
                    buildConflictReport(slot), decision.reason());
        }

        return SaveUploadResponse.accepted(
                decision.outcome(), slot.getHeadVersion(), slot.getVectorClock(), decision.reason());
    }

    // ── internals ───────────────────────────────────────────────────────────

    private void recordSuspicionIfNeeded(UUID playerId, Boolean clientHmacValid) {
        if (clientHmacValid != null && !clientHmacValid) {
            playerRepository.findByIdForUpdate(playerId).ifPresent(player -> {
                player.raiseIntegritySuspicion(SaveSigner.CLIENT_HMAC_MISMATCH_WEIGHT);
                log.info("Client HMAC mismatch for player={}; suspicion now {}",
                        playerId, player.getIntegrityScore());
            });
        }
    }

    private SaveUploadResponse.ConflictReport buildConflictReport(SaveSlot slot) {
        return versionRepository.findLatestConflictLoser(slot.getId())
                .map(loser -> new SaveUploadResponse.ConflictReport(
                        loser.getVersion(),
                        slot.getEpisodeId(),
                        slot.getPlaytimeSeconds(),
                        loser.getOriginDeviceId(),
                        slot.getHandIntegrity(),
                        true))
                .orElse(null);
    }

    static int episodeNumberOf(String episodeId) {
        if (episodeId == null || episodeId.length() != 5) {
            return 0;
        }
        try {
            return Integer.parseInt(episodeId.substring(2));
        } catch (NumberFormatException e) {
            return 0;
        }
    }
}
