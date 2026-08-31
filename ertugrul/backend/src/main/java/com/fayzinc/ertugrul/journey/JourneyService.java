package com.fayzinc.ertugrul.journey;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.identity.PlayerRepository;
import com.fayzinc.ertugrul.journey.dto.JourneyEntryRequest;
import com.fayzinc.ertugrul.journey.dto.JourneyEntryResponse;
import com.fayzinc.ertugrul.journey.dto.PublicJourneyResponse;
import com.fayzinc.ertugrul.journey.dto.ShareLinkResponse;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.security.SecureRandom;
import java.time.Instant;
import java.util.Base64;
import java.util.List;
import java.util.UUID;

/**
 * Safar Daftari: writing pages, listing diaries, and public share links.
 *
 * <p>PDF eksporti alohida: {@link JourneyExportService}.
 */
@Service
public class JourneyService {

    private static final Logger log = LoggerFactory.getLogger(JourneyService.class);

    private static final SecureRandom RANDOM = new SecureRandom();

    /**
     * Share token entropy.
     *
     * <p>16 bayt = 128 bit. Ommaviy havolada yagona himoya — topib
     * bo'lmaslik, shuning uchun bu yerda tejash mumkin emas.
     */
    private static final int SHARE_TOKEN_BYTES = 16;

    /** Public base URL the share link is built on. */
    private static final String SHARE_BASE_URL = "https://dirilis-game.com";

    private final JourneyRepository.JourneyEntryRepository entryRepository;
    private final JourneyRepository.JourneyShareRepository shareRepository;
    private final PlayerRepository playerRepository;
    private final ErtugrulProperties props;

    public JourneyService(JourneyRepository.JourneyEntryRepository entryRepository,
                          JourneyRepository.JourneyShareRepository shareRepository,
                          PlayerRepository playerRepository,
                          ErtugrulProperties props) {
        this.entryRepository = entryRepository;
        this.shareRepository = shareRepository;
        this.playerRepository = playerRepository;
        this.props = props;
    }

    /**
     * Daftarga yangi sahifa yozadi.
     *
     * <p>Idempotent: {@code (playthroughId, sequenceNo)} unikal, shuning uchun
     * tarmoq uzilishidan keyin klientning qayta urinishi <b>dublikat
     * yaratmaydi</b> — mavjud sahifa qaytariladi. Konsolda tarmoq uzilishi
     * odatiy hol, shuning uchun bu tasodifiy emas, majburiy xususiyat.
     *
     * @param playerId the diary's owner
     * @param request  the page to write
     * @return the stored page, whether newly written or already present
     * @throws ErtugrulException {@code JOURNEY_QUOTA_EXCEEDED} past the per-player cap
     */
    @Transactional
    public JourneyEntryResponse writeEntry(UUID playerId, JourneyEntryRequest request) {

        // Idempotency first: cheaper than the quota count, and the common retry
        // path should not be charged for work it does not need.
        var existing = entryRepository.findByPlaythroughIdAndSequenceNo(
                request.playthroughId(), request.sequenceNo());
        if (existing.isPresent()) {
            return JourneyEntryResponse.from(existing.get());
        }

        long written = entryRepository.countByPlayerId(playerId);
        if (written >= props.journey().maxEntriesPerPlayer()) {
            throw new ErtugrulException(ErtugrulException.Code.JOURNEY_QUOTA_EXCEEDED,
                    "Diary limit of %d pages reached".formatted(props.journey().maxEntriesPerPlayer()));
        }

        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));

        JourneyEntry entry = new JourneyEntry(
                player,
                request.playthroughId(),
                request.sequenceNo(),
                request.episodeId(),
                request.seasonId(),
                request.hijriDateText(),
                request.gregorianDateText(),
                request.inGameDate(),
                request.body());

        entry.setPlaceName(request.placeName());
        entry.setLinkedCodexIds(request.linkedCodexIds());
        entry.setTone(request.tone());
        entry.setWrittenLeftHanded(request.writtenLeftHanded());
        entry.setOriginDeviceId(request.deviceId());

        try {
            return JourneyEntryResponse.from(entryRepository.saveAndFlush(entry));
        } catch (DataIntegrityViolationException race) {
            // Two devices wrote the same page number at once. The unique
            // constraint decided; read the winner back rather than failing.
            return entryRepository
                    .findByPlaythroughIdAndSequenceNo(request.playthroughId(), request.sequenceNo())
                    .map(JourneyEntryResponse::from)
                    .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.INTERNAL,
                            "Journey entry vanished after unique-constraint violation", race));
        }
    }

    /** Every page of one diary, in order. */
    @Transactional(readOnly = true)
    public List<JourneyEntryResponse> readDiary(UUID playerId, UUID playthroughId) {
        return entryRepository
                .findByPlayerIdAndPlaythroughIdOrderBySequenceNoAsc(playerId, playthroughId).stream()
                .map(JourneyEntryResponse::from)
                .toList();
    }

    /**
     * O'yinchining barcha daftarlari.
     *
     * <p>NG+ har safar yangi daftar boshlaydi, shuning uchun o'yinchida bir
     * nechta bo'lishi mumkin va u qaysi birini ko'rishni tanlaydi.
     */
    @Transactional(readOnly = true)
    public List<DiarySummary> listDiaries(UUID playerId) {
        return entryRepository.summarisePlaythroughs(playerId).stream()
                .map(row -> new DiarySummary(
                        (UUID) row[0],
                        (Instant) row[1],
                        ((Number) row[2]).intValue(),
                        ((Number) row[3]).longValue()))
                .toList();
    }

    /** Hides or unhides a page from shared links, without deleting it. */
    @Transactional
    public void setPageHidden(UUID playerId, UUID entryId, boolean hidden) {
        JourneyEntry entry = entryRepository.findById(entryId)
                .orElseThrow(() -> new ErtugrulException(
                        ErtugrulException.Code.JOURNEY_ENTRY_NOT_FOUND, "No such diary page"));

        if (!entry.getPlayer().getId().equals(playerId)) {
            throw new ErtugrulException(ErtugrulException.Code.JOURNEY_ENTRY_NOT_FOUND, "No such diary page");
        }
        entry.setHiddenFromShare(hidden);
    }

    /**
     * Ommaviy havola yaratadi.
     *
     * <p>Havola muddatli va bekor qilinadigan. U <b>faqat daftar matnini</b>
     * ochadi — o'yinchi ID'si, save ma'lumoti yoki boshqa shaxsiy narsa
     * hech qachon chiqmaydi.
     *
     * @param playerId      the diary's owner
     * @param playthroughId which diary to share
     * @param publicTitle   what viewers see as the title
     * @param fromSeq       first page to share, or null for the beginning
     * @param toSeq         last page, or null for "everything so far"
     * @return the created link
     */
    @Transactional
    public ShareLinkResponse createShareLink(UUID playerId, UUID playthroughId,
                                             String publicTitle, Integer fromSeq, Integer toSeq) {

        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));

        List<JourneyEntry> pages = entryRepository
                .findByPlayerIdAndPlaythroughIdOrderBySequenceNoAsc(playerId, playthroughId);
        if (pages.isEmpty()) {
            throw new ErtugrulException(ErtugrulException.Code.JOURNEY_ENTRY_NOT_FOUND,
                    "This diary has no pages to share");
        }

        JourneyShare share = new JourneyShare(
                player, playthroughId, generateShareToken(),
                Instant.now().plus(props.journey().shareLinkTtl()));

        share.setPublicTitle(publicTitle);
        share.setFromSequenceNo(fromSeq);
        share.setToSequenceNo(toSeq);

        JourneyShare saved = shareRepository.save(share);
        log.info("Created diary share link for player={} playthrough={}", playerId, playthroughId);

        return ShareLinkResponse.from(saved, SHARE_BASE_URL);
    }

    /**
     * Ommaviy havolani ochadi — <b>autentifikatsiyasiz</b>.
     *
     * <p>Bu servisdagi yagona autentifikatsiyasiz yo'l. Shuning uchun u
     * ataylab juda tor: token bo'yicha bitta qator topiladi, tiriklik
     * tekshiriladi, va faqat yashirilmagan sahifalar qaytariladi. Boshqa
     * hech narsa.
     *
     * @param shareToken the token from the URL
     * @return the shared diary
     * @throws ErtugrulException {@code SHARE_LINK_NOT_FOUND} or {@code SHARE_LINK_EXPIRED}
     */
    @Transactional
    public PublicJourneyResponse readShared(String shareToken) {
        JourneyShare share = shareRepository.findByShareToken(shareToken)
                .orElseThrow(() -> new ErtugrulException(
                        ErtugrulException.Code.SHARE_LINK_NOT_FOUND, "Unknown share link"));

        if (!share.isLive(Instant.now())) {
            throw new ErtugrulException(ErtugrulException.Code.SHARE_LINK_EXPIRED,
                    "This share link has expired or been revoked");
        }

        // hiddenFromShare is filtered in the query, not in Java: a page the
        // player chose to hide must not depend on a stream filter somebody could
        // later reorder away.
        List<JourneyEntry> pages = entryRepository.findShareable(
                share.getPlaythroughId(), share.getFromSequenceNo(), share.getToSequenceNo());

        share.recordView();

        return new PublicJourneyResponse(
                share.getPublicTitle() == null ? "Safar Daftari" : share.getPublicTitle(),
                pages.size(),
                pages.stream().map(PublicJourneyResponse.PublicPage::from).toList(),
                share.getCreatedAt());
    }

    /** Revokes a share link; the URL immediately stops resolving. */
    @Transactional
    public void revokeShareLink(UUID playerId, UUID shareId) {
        JourneyShare share = shareRepository.findById(shareId)
                .orElseThrow(() -> new ErtugrulException(
                        ErtugrulException.Code.SHARE_LINK_NOT_FOUND, "No such share link"));

        if (!share.getPlayer().getId().equals(playerId)) {
            throw new ErtugrulException(ErtugrulException.Code.SHARE_LINK_NOT_FOUND, "No such share link");
        }
        share.revoke();
        log.info("Revoked diary share link {} for player={}", shareId, playerId);
    }

    /** Every share link the player has created, live or not. */
    @Transactional(readOnly = true)
    public List<ShareLinkResponse> listShareLinks(UUID playerId) {
        return shareRepository.findByPlayerId(playerId).stream()
                .map(share -> ShareLinkResponse.from(share, SHARE_BASE_URL))
                .toList();
    }

    // ── internals ───────────────────────────────────────────────────────────

    private static String generateShareToken() {
        byte[] bytes = new byte[SHARE_TOKEN_BYTES];
        RANDOM.nextBytes(bytes);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(bytes);
    }

    /**
     * One diary in the player's list.
     *
     * @param playthroughId which diary
     * @param startedAt     when its first page was written
     * @param lastSequenceNo highest page number
     * @param pageCount     how many pages it holds
     */
    public record DiarySummary(UUID playthroughId, Instant startedAt,
                               int lastSequenceNo, long pageCount) {
    }
}
