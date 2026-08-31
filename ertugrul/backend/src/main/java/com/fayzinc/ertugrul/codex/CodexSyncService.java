package com.fayzinc.ertugrul.codex;

import com.fayzinc.ertugrul.codex.dto.CodexSyncRequest;
import com.fayzinc.ertugrul.codex.dto.CodexSyncResponse;
import com.fayzinc.ertugrul.codex.dto.CodexUnlockDto;
import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.identity.PlayerRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.ArrayList;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.stream.Collectors;

/**
 * Cross-device codex synchronisation.
 *
 * <p><b>Nega bu save'dan ko'ra ancha sodda.</b> Kodeks progressi — <i>o'suvchi
 * to'plam</i> (grow-only set): yozuv ochilgach hech qachon qayta qulflanmaydi.
 * Matematik jihatdan bu CRDT, ya'ni birlashtirish amali kommutativ, assotsiativ
 * va idempotent. Amalda bu shuni anglatadiki, <b>konflikt umuman bo'lishi
 * mumkin emas</b> va vector clock kerak emas — oddiy birlashma (union) yetadi.
 *
 * <p>Faqat ikkita o'zgaruvchan maydon bor ({@code readCount},
 * {@code bookmarked}), ular {@code revision} bo'yicha LWW bilan hal qilinadi.
 */
@Service
public class CodexSyncService {

    private static final Logger log = LoggerFactory.getLogger(CodexSyncService.class);

    private final CodexRepository codexRepository;
    private final PlayerRepository playerRepository;

    public CodexSyncService(CodexRepository codexRepository, PlayerRepository playerRepository) {
        this.codexRepository = codexRepository;
        this.playerRepository = playerRepository;
    }

    /**
     * Ikki yo'nalishli sinxronni bitta so'rovda bajaradi.
     *
     * <p>Bosqichlar:
     * <ol>
     *   <li>Klient yuborgan yozuvlar birlashtiriladi — yangilari qo'shiladi,
     *       mavjudlari {@link CodexEntryProgress#mergeFrom} bilan yangilanadi;</li>
     *   <li>Serverdagi, klientda yo'q bo'lgan yozuvlar qaytariladi;</li>
     *   <li>Kollektsiya statistikasi hisoblanadi.</li>
     * </ol>
     *
     * <p>Idempotent: bir xil so'rovni ikki marta yuborish natijani
     * o'zgartirmaydi. Bu muhim, chunki konsolda tarmoq uzilishi keng tarqalgan
     * va klient qayta urinadi.
     *
     * @param playerId the syncing player
     * @param request  the client's unlocks and last sync watermark
     * @return entries the client is missing, plus collection statistics
     * @throws ErtugrulException when the player does not exist
     */
    @Transactional
    public CodexSyncResponse sync(UUID playerId, CodexSyncRequest request) {

        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));

        // Honours Settings -> Account & Cloud -> Codex sync [ON/OFF]. When the
        // player opted out we still answer successfully, just with nothing —
        // failing here would make the client retry forever.
        if (!player.isCodexSyncEnabled()) {
            log.debug("Codex sync disabled for player={}; returning empty response", playerId);
            return new CodexSyncResponse(List.of(), 0, 0, Map.of(), Map.of(), Instant.now());
        }

        int accepted = mergeClientUnlocks(player, request);

        List<CodexUnlockDto> toSend = collectServerSideUnlocks(playerId, request);

        return new CodexSyncResponse(
                toSend,
                accepted,
                codexRepository.countByPlayerId(playerId),
                categoryCounts(playerId),
                unreadCounts(playerId),
                Instant.now());
    }

    /**
     * Bitta yozuvni "o'qildi" deb belgilaydi.
     *
     * <p>Bu chaqiruv <i>hodisa</i>, holat emas: klient har ochilishda yuboradi.
     * Shuning uchun u sinxrondan alohida va juda yengil.
     *
     * @param playerId the reader
     * @param codexId  which entry
     * @throws ErtugrulException when the entry was never unlocked
     */
    @Transactional
    public void markRead(UUID playerId, String codexId) {
        CodexEntryProgress entry = codexRepository.findByPlayerIdAndCodexId(playerId, codexId)
                .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.CODEX_ENTRY_UNKNOWN,
                        "Codex entry not unlocked for this player: " + codexId));
        entry.markRead();
    }

    /** Toggles a bookmark on an unlocked entry. */
    @Transactional
    public void setBookmark(UUID playerId, String codexId, boolean bookmarked) {
        CodexEntryProgress entry = codexRepository.findByPlayerIdAndCodexId(playerId, codexId)
                .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.CODEX_ENTRY_UNKNOWN,
                        "Codex entry not unlocked for this player: " + codexId));
        entry.setBookmarked(bookmarked);
    }

    /** Full snapshot — used on a fresh install or after "restore progress". */
    @Transactional(readOnly = true)
    public List<CodexUnlockDto> fullSnapshot(UUID playerId) {
        return codexRepository.findByPlayerId(playerId).stream()
                .map(CodexUnlockDto::from)
                .toList();
    }

    // ── internals ───────────────────────────────────────────────────────────

    /**
     * Klient yozuvlarini serverga birlashtiradi.
     *
     * @return how many entries were genuinely new to the server
     */
    private int mergeClientUnlocks(Player player, CodexSyncRequest request) {
        if (request.unlocks().isEmpty()) {
            return 0;
        }

        Set<String> incomingIds = request.unlocks().stream()
                .map(CodexUnlockDto::codexId)
                .collect(Collectors.toSet());

        // One batched read instead of one query per entry — this is the single
        // most important line in the method for sync latency.
        Map<String, CodexEntryProgress> existing =
                codexRepository.findByPlayerIdAndCodexIdIn(player.getId(), incomingIds).stream()
                        .collect(Collectors.toMap(CodexEntryProgress::getCodexId, e -> e));

        List<CodexEntryProgress> toInsert = new ArrayList<>();
        int newlyUnlocked = 0;

        for (CodexUnlockDto dto : request.unlocks()) {
            CodexEntryProgress current = existing.get(dto.codexId());

            if (current == null) {
                toInsert.add(new CodexEntryProgress(
                        player,
                        dto.codexId(),
                        dto.confidence(),
                        dto.category(),
                        dto.unlockMethod(),
                        dto.episodeId(),
                        dto.unlockedAt() == null ? Instant.now() : dto.unlockedAt(),
                        request.deviceId()));
                newlyUnlocked++;
            } else {
                current.mergeFrom(dto.readCount(), dto.bookmarked(), dto.revision(),
                        dto.unlockedAt(), request.deviceId());
            }
        }

        if (!toInsert.isEmpty()) {
            codexRepository.saveAll(toInsert);
            log.debug("Codex sync: {} new unlocks for player={}", newlyUnlocked, player.getId());
        }

        return newlyUnlocked;
    }

    /**
     * Klientda yo'q bo'lgan server yozuvlarini yig'adi.
     *
     * <p>{@code since} berilgan bo'lsa faqat farq yuboriladi; aks holda to'liq
     * suratdan klient allaqachon bilganlari chiqarib tashlanadi.
     */
    private List<CodexUnlockDto> collectServerSideUnlocks(UUID playerId, CodexSyncRequest request) {
        Set<String> clientKnows = request.unlocks().stream()
                .map(CodexUnlockDto::codexId)
                .collect(Collectors.toSet());

        List<CodexEntryProgress> candidates = request.since() == null
                ? codexRepository.findByPlayerId(playerId)
                : codexRepository.findByPlayerIdAndUpdatedAtAfter(playerId, request.since());

        return candidates.stream()
                .filter(entry -> !clientKnows.contains(entry.getCodexId()))
                .map(CodexUnlockDto::from)
                .toList();
    }

    private Map<CodexCategory, Long> categoryCounts(UUID playerId) {
        Map<CodexCategory, Long> counts = new EnumMap<>(CodexCategory.class);
        for (Object[] row : codexRepository.countByCategoryForPlayer(playerId)) {
            counts.put((CodexCategory) row[0], (Long) row[1]);
        }
        return counts;
    }

    private Map<CodexEntryProgress.Confidence, Long> unreadCounts(UUID playerId) {
        Map<CodexEntryProgress.Confidence, Long> counts = new HashMap<>();
        for (Object[] row : codexRepository.countUnreadByConfidence(playerId)) {
            counts.put((CodexEntryProgress.Confidence) row[0], (Long) row[1]);
        }
        return counts;
    }
}
