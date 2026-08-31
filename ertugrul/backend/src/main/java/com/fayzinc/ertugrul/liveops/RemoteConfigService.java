package com.fayzinc.ertugrul.liveops;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.config.RedisConfig;
import com.fayzinc.ertugrul.save.DifficultyTier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.redis.core.RedisTemplate;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Resolves the live-ops configuration document for a player.
 *
 * <p><b>Kogort taqsimoti — eng nozik joyi.</b> A/B tajribasi ishlashi uchun
 * o'yinchi <i>barqaror</i> guruhda qolishi shart. Agar u har sessiyada
 * tasodifiy guruhga tushsa: (a) natijalar aralashib ketadi va tajriba hech
 * narsa isbotlamaydi, (b) o'yinchi bir kun oson, ertasiga qiyin balansni
 * ko'radi va o'yinni buzuq deb hisoblaydi.
 *
 * <p>Shuning uchun guruh <b>hisoblanadi, saqlanmaydi</b>:
 * {@code SHA-256(playerId + salt) mod 100}. Bu determinlashtirilgan, holatsiz
 * va har serverda bir xil natija beradi. Yagona shart —
 * {@code ertugrul.liveops.cohort-salt} tajriba davomida <b>o'zgarmasligi</b>:
 * uni o'zgartirish barcha taqsimotni qaytadan aralashtiradi.
 */
@Service
public class RemoteConfigService {

    private static final Logger log = LoggerFactory.getLogger(RemoteConfigService.class);

    private static final int COHORT_BUCKETS = 100;

    private final LiveOpsRepositories.RemoteConfigRepository configRepository;
    private final LiveOpsRepositories.EpisodeBalanceOverrideRepository overrideRepository;
    private final LiveOpsRepositories.SeasonalEventRepository eventRepository;
    private final RedisTemplate<String, Object> redis;
    private final ErtugrulProperties props;

    public RemoteConfigService(LiveOpsRepositories.RemoteConfigRepository configRepository,
                               LiveOpsRepositories.EpisodeBalanceOverrideRepository overrideRepository,
                               LiveOpsRepositories.SeasonalEventRepository eventRepository,
                               RedisTemplate<String, Object> jsonRedisTemplate,
                               ErtugrulProperties props) {
        this.configRepository = configRepository;
        this.overrideRepository = overrideRepository;
        this.eventRepository = eventRepository;
        this.redis = jsonRedisTemplate;
        this.props = props;
    }

    /**
     * O'yinchining barqaror kogort guruhini hisoblaydi.
     *
     * <p>Saqlanmaydi — har safar bir xil natija beradigan sof funksiya. Bu
     * shuni anglatadiki, yangi server ko'tarilganda ham, baza tiklanganda ham
     * o'yinchi o'sha guruhda qoladi.
     *
     * @param playerId the player
     * @return a stable bucket in {@code 0..99}
     */
    public int cohortBucket(UUID playerId) {
        byte[] digest = sha256((playerId + ":" + props.liveops().cohortSalt())
                .getBytes(StandardCharsets.UTF_8));

        // First four bytes as an unsigned int, then modulo. Using the raw hash
        // rather than String.hashCode() matters: hashCode is not stable across
        // JVM versions, and a reshuffle would silently invalidate every
        // running experiment.
        int value = ((digest[0] & 0xFF) << 24)
                | ((digest[1] & 0xFF) << 16)
                | ((digest[2] & 0xFF) << 8)
                | (digest[3] & 0xFF);

        return Math.floorMod(value, COHORT_BUCKETS);
    }

    /**
     * O'yinchi uchun to'liq live-ops hujjatini qaytaradi.
     *
     * <p>Natija Redis'da {@code ertugrul.liveops.remote-config-cache-ttl}
     * davomida saqlanadi. Kesh kaliti — kogort guruhi va klient versiyasi,
     * o'yinchi ID'si emas: bir guruhdagi hamma bir xil hujjatni oladi, ya'ni
     * 100 ta kesh yozuvi millionlab o'yinchiga xizmat qiladi.
     *
     * @param playerId   the requesting player
     * @param appVersion client build, for version targeting
     * @param tier       difficulty this playthrough runs at
     * @return the resolved document the client should apply
     */
    @Transactional(readOnly = true)
    public LiveOpsDocument resolve(UUID playerId, String appVersion, DifficultyTier tier) {
        int bucket = cohortBucket(playerId);
        String cacheKey = RedisConfig.Keys.remoteConfig(bucket, safeVersion(appVersion));

        try {
            Object cached = redis.opsForValue().get(cacheKey);
            if (cached instanceof LiveOpsDocument document) {
                return document;
            }
        } catch (Exception e) {
            // Redis down: fall through to the database. Live-ops must never be
            // the reason a player cannot start the game.
            log.debug("Remote config cache read failed; falling back to database", e);
        }

        LiveOpsDocument document = buildDocument(bucket, appVersion, tier);

        try {
            redis.opsForValue().set(cacheKey, document, props.liveops().remoteConfigCacheTtl());
        } catch (Exception e) {
            log.debug("Remote config cache write failed; serving uncached", e);
        }

        return document;
    }

    /**
     * Bitta epizod uchun balans override'ini qaytaradi.
     *
     * <p>Klient epizodni yuklashdan oldin so'raydi. Agar bir epizodda bir necha
     * variant bo'lsa (A/B), o'yinchiga faqat o'z kogortiga mos keladigani
     * beriladi; bir nechtasi mos kelsa — birinchisi, chunki bir vaqtda ikki
     * balans qo'llanishi mumkin emas.
     *
     * @param playerId  the requesting player
     * @param episodeId EP001..EP048
     * @param tier      difficulty this playthrough runs at
     * @return the override to apply, or null when the episode runs at baseline
     */
    @Transactional(readOnly = true)
    public BalanceOverrideView balanceFor(UUID playerId, String episodeId, DifficultyTier tier) {
        int bucket = cohortBucket(playerId);
        Instant now = Instant.now();

        return overrideRepository.findByEpisodeIdAndEnabledTrue(episodeId).stream()
                .filter(override -> override.appliesTo(bucket, tier, now))
                .findFirst()
                .map(BalanceOverrideView::from)
                .orElse(null);
    }

    /**
     * Balans override'ini yaratadi yoki yangilaydi — live-ops paneli chaqiradi.
     *
     * @param episodeId   the episode to tune
     * @param variantKey  experiment variant, or {@code default}
     * @param parryDelta  parry window adjustment, -60..120 ms
     * @param damageScale enemy damage multiplier, 0.25..2.5
     * @param woundScale  wound decay multiplier, 0.2..2.0
     * @param countScale  enemy count multiplier, 0.5..2.0
     * @param rollout     cohort percentage, 0..100
     * @param editor      who made the change; kept for the audit trail
     * @return the stored override
     * @throws ErtugrulException when the episode id is malformed
     */
    @Transactional
    public BalanceOverrideView upsertBalance(String episodeId, String variantKey,
                                             int parryDelta, float damageScale,
                                             float woundScale, float countScale,
                                             int rollout, String editor) {

        if (!episodeId.matches("^EP0(0[1-9]|[1-3][0-9]|4[0-8])$")) {
            throw new ErtugrulException(ErtugrulException.Code.EPISODE_UNKNOWN,
                    "episodeId must be EP001..EP048, got " + episodeId);
        }

        EpisodeBalanceOverride override = overrideRepository
                .findByEpisodeIdAndVariantKey(episodeId, variantKey)
                .orElseGet(() -> new EpisodeBalanceOverride(episodeId, variantKey));

        override.retune(parryDelta, damageScale, woundScale, countScale, editor);
        override.setRollout((short) rollout);
        override.setEnabled(true);

        EpisodeBalanceOverride saved = overrideRepository.save(override);

        // The document cache is keyed by cohort, so a balance change must not
        // wait out the TTL before players see it.
        evictDocumentCache();

        log.info("Balance override upserted: episode={} variant={} rollout={}% by {}",
                episodeId, variantKey, rollout, editor);

        return BalanceOverrideView.from(saved);
    }

    // ── internals ───────────────────────────────────────────────────────────

    private LiveOpsDocument buildDocument(int bucket, String appVersion, DifficultyTier tier) {
        Instant now = Instant.now();

        Map<String, Object> configs = new HashMap<>();
        for (RemoteConfig config : configRepository.findActive(now)) {
            if (config.appliesToCohort(bucket) && versionSatisfies(appVersion, config.getMinAppVersion())) {
                configs.put(config.getConfigKey(), config.getConfigValue());
            }
        }

        List<BalanceOverrideView> balances = overrideRepository.findByEnabledTrue().stream()
                .filter(override -> override.appliesTo(bucket, tier, now))
                .map(BalanceOverrideView::from)
                .toList();

        List<LiveEventView> events = eventRepository.findLive(now).stream()
                .map(LiveEventView::from)
                .toList();

        // ETag input: the client sends it back and gets a 304 when nothing moved.
        long revision = configRepository.maxActiveRevision(now)
                + overrideRepository.maxEnabledRevision();

        return new LiveOpsDocument(bucket, configs, balances, events,
                Long.toString(revision), now);
    }

    /**
     * Butun hujjat keshini tozalaydi.
     *
     * <p>Kesh kogort va klient versiyasi bo'yicha kalitlanadi, ya'ni bitta
     * balans o'zgarishi o'nlab kalitga tegadi. Ularni birma-bir hisoblab
     * chiqishdan ko'ra naqsh bo'yicha o'chirish soddaroq, va bu amal kuniga
     * bir necha marta bajariladi — {@code KEYS} ning narxi bu chastotada
     * ahamiyatsiz.
     */
    private void evictDocumentCache() {
        try {
            var keys = redis.keys(RedisConfig.Keys.remoteConfigPattern());
            if (keys != null && !keys.isEmpty()) {
                redis.delete(keys);
                log.debug("Evicted {} remote config cache entries", keys.size());
            }
        } catch (Exception e) {
            log.debug("Could not evict remote config cache; entries will expire on TTL", e);
        }
    }

    /**
     * Semantic-ish version gate.
     *
     * <p>Ataylab yumshoq: noto'g'ri formatdagi versiya <b>o'tkaziladi</b>, rad
     * etilmaydi. Sabab — noto'g'ri versiya satri tufayli o'yinchini
     * konfiguratsiyasiz qoldirish, uni ortiqcha konfiguratsiya berishdan
     * ancha yomonroq.
     */
    private static boolean versionSatisfies(String actual, String minimum) {
        if (minimum == null || minimum.isBlank() || actual == null || actual.isBlank()) {
            return true;
        }
        try {
            String[] a = actual.split("[+\\-]")[0].split("\\.");
            String[] m = minimum.split("[+\\-]")[0].split("\\.");
            for (int i = 0; i < Math.max(a.length, m.length); i++) {
                int actualPart = i < a.length ? Integer.parseInt(a[i]) : 0;
                int minPart = i < m.length ? Integer.parseInt(m[i]) : 0;
                if (actualPart != minPart) {
                    return actualPart > minPart;
                }
            }
            return true;
        } catch (NumberFormatException e) {
            return true;
        }
    }

    private static String safeVersion(String appVersion) {
        return appVersion == null || appVersion.isBlank() ? "unknown" : appVersion;
    }

    private static byte[] sha256(byte[] input) {
        try {
            return MessageDigest.getInstance("SHA-256").digest(input);
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
    }

    // ── Views ───────────────────────────────────────────────────────────────

    /**
     * The whole live-ops payload for one player.
     *
     * @param cohortBucket the player's stable bucket; echoed so QA can reproduce a report
     * @param configs      arbitrary remote config values, keyed by config key
     * @param balances     per-episode balance overrides that apply to this player
     * @param liveEvents   seasonal events currently running
     * @param revision     ETag; send back as {@code If-None-Match}
     * @param generatedAt  server time the document was built
     */
    public record LiveOpsDocument(
            int cohortBucket,
            Map<String, Object> configs,
            List<BalanceOverrideView> balances,
            List<LiveEventView> liveEvents,
            String revision,
            Instant generatedAt
    ) {
    }

    /**
     * One episode's balance override, as the client consumes it.
     *
     * @param episodeId          which episode
     * @param variantKey         experiment variant
     * @param parryWindowMsDelta added to the base parry window
     * @param enemyDamageScale   multiplier on incoming damage
     * @param woundDecayScale    multiplier on HandIntegrity decay
     * @param enemyCountScale    multiplier on encounter size
     */
    public record BalanceOverrideView(
            String episodeId,
            String variantKey,
            int parryWindowMsDelta,
            float enemyDamageScale,
            float woundDecayScale,
            float enemyCountScale
    ) {
        public static BalanceOverrideView from(EpisodeBalanceOverride entity) {
            return new BalanceOverrideView(
                    entity.getEpisodeId(),
                    entity.getVariantKey(),
                    entity.getParryWindowMsDelta(),
                    entity.getEnemyDamageScale(),
                    entity.getWoundDecayScale(),
                    entity.getEnemyCountScale());
        }
    }

    /**
     * A running seasonal event.
     *
     * @param eventKey       stable identifier
     * @param eventType      what kind of event
     * @param titleLocKey    localisation key for the title
     * @param bodyLocKey     localisation key for the body
     * @param cdnManifestUrl for codex drops, where the new entries live
     * @param endsAt         when it stops
     */
    public record LiveEventView(
            String eventKey,
            SeasonalEvent.EventType eventType,
            String titleLocKey,
            String bodyLocKey,
            String cdnManifestUrl,
            Instant endsAt
    ) {
        public static LiveEventView from(SeasonalEvent entity) {
            return new LiveEventView(
                    entity.getEventKey(),
                    entity.getEventType(),
                    entity.getTitleLocKey(),
                    entity.getBodyLocKey(),
                    entity.getCdnManifestUrl(),
                    entity.getEndsAt());
        }
    }
}
