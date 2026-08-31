package com.fayzinc.ertugrul.telemetry.funnel;

import com.fayzinc.ertugrul.save.DifficultyTier;
import com.fayzinc.ertugrul.telemetry.TelemetryEvent;
import jakarta.persistence.EntityManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDate;
import java.time.ZoneOffset;
import java.util.List;

/**
 * Rolls telemetry up into the per-episode funnel and the wound-balance table.
 *
 * <p><b>Nima uchun bu bor.</b> 00_AUDIT.md D8: "telemetriya hook'lari yo'q →
 * retention'ni o'lchay olmaysiz". Bu servis aynan shu bo'shliqni yopadi. U
 * ikki savolga javob beradi:
 * <ol>
 *   <li><b>Qayerda tashlab ketishyapti?</b> — {@code completes/starts}
 *       nisbati har epizod uchun;</li>
 *   <li><b>Jarohat tizimi qayerda juda qattiq?</b> — 05_MIH_SYSTEM.md §8 dagi
 *       chegara bo'yicha avtomatik aniqlash.</li>
 *  </ol>
 *
 * <p><b>Nega native UPSERT, JPA emas.</b> Rollup'lar — sof qo'shimcha
 * (additive) hisoblagichlar. JPA orqali "o'qi → o'zgartir → yoz" qilsak,
 * parallel iste'molchilar bir-birining o'zgarishini bosib ketadi (lost update),
 * va buni oldini olish uchun qulf kerak bo'ladi — bu esa iste'mol tezligini
 * o'ldiradi. {@code INSERT ... ON CONFLICT DO UPDATE SET x = x + n} atomik va
 * qulfsiz.
 */
@Service
public class EpisodeFunnelService {

    private static final Logger log = LoggerFactory.getLogger(EpisodeFunnelService.class);

    /**
     * "Juda qattiq" chegarasi — 05_MIH_SYSTEM.md §8.
     *
     * <p>Hujjatdagi qoida: agar epizodda o'rtacha {@code hand_integrity < 15}
     * va o'rtacha o'lim {@code > 6} bo'lsa — tizim o'sha yerda juda qattiq va
     * live-ops orqali balans patch'i kerak.
     */
    public static final float PUNISHING_INTEGRITY_THRESHOLD = 15.0f;
    public static final double PUNISHING_DEATHS_THRESHOLD = 6.0;

    /** Below this many samples the average is noise, not a signal. */
    private static final long MIN_SAMPLES_FOR_FLAG = 50;

    /**
     * Constructor-injected rather than {@code @PersistenceContext}: Spring hands
     * over the shared transactional proxy either way, and this keeps the field
     * final and the class testable without a container.
     */
    private final EntityManager entityManager;

    public EpisodeFunnelService(EntityManager entityManager) {
        this.entityManager = entityManager;
    }

    /**
     * Bir to'plam hodisani rollup jadvallariga qo'shadi.
     *
     * <p>Telemetriya iste'molchisi (Kafka consumer) chaqiradi. Idempotent
     * emas — bu ataylab: hodisalar {@code eventId} bo'yicha iste'molchida
     * deduplikatsiya qilinadi, bu yerda esa faqat tez qo'shish bo'ladi.
     *
     * @param events a consumed batch, already deduplicated upstream
     */
    @Transactional
    public void accumulate(List<TelemetryEvent> events) {
        for (TelemetryEvent event : events) {
            if (event.episodeId() == null) {
                continue;
            }
            switch (event.type()) {
                case EPISODE_START -> bumpFunnel(event, "starts", 1);
                case EPISODE_COMPLETE -> recordCompletion(event);
                case EPISODE_ABANDON -> bumpFunnel(event, "abandons", 1);
                case DEATH -> bumpFunnel(event, "deaths", 1);
                case INTRO_SKIPPED -> bumpFunnel(event, "intro_skips", 1);
                case WOUND_STATE -> accumulateWound(event);
                default -> {
                    // CHOICE_MADE, CODEX_UNLOCK and SETTINGS_CHANGED are rolled up
                    // elsewhere: choices in ChoiceAggregateService, the rest in the
                    // warehouse. Nothing to do here.
                }
            }
        }
    }

    /**
     * Bitta funnel hisoblagichini oshiradi.
     *
     * <p>Ustun nomi tashqaridan kelmaydi — u faqat shu sinf ichidagi
     * konstantalardan biri, shuning uchun bu yerda SQL injection yo'li yo'q.
     */
    private void bumpFunnel(TelemetryEvent event, String column, long delta) {
        String sql = """
                insert into episode_funnel_daily
                    (bucket_date, episode_id, difficulty_tier, %s, updated_at)
                values (:bucketDate, :episodeId, :tier, :delta, now())
                on conflict (bucket_date, episode_id, difficulty_tier)
                do update set %s = episode_funnel_daily.%s + :delta,
                              updated_at = now()
                """.formatted(column, column, column);

        entityManager.createNativeQuery(sql)
                .setParameter("bucketDate", bucketDate(event))
                .setParameter("episodeId", event.episodeId())
                .setParameter("tier", tierOf(event))
                .setParameter("delta", delta)
                .executeUpdate();
    }

    /** Completion also carries a duration, which needs a sum + count pair. */
    private void recordCompletion(TelemetryEvent event) {
        long durationSec = parseLongAttribute(event, "duration_sec");

        entityManager.createNativeQuery("""
                        insert into episode_funnel_daily
                            (bucket_date, episode_id, difficulty_tier,
                             completes, duration_sum_sec, duration_count, updated_at)
                        values (:bucketDate, :episodeId, :tier, 1, :duration, 1, now())
                        on conflict (bucket_date, episode_id, difficulty_tier)
                        do update set completes        = episode_funnel_daily.completes + 1,
                                      duration_sum_sec = episode_funnel_daily.duration_sum_sec + :duration,
                                      duration_count   = episode_funnel_daily.duration_count + 1,
                                      updated_at       = now()
                        """)
                .setParameter("bucketDate", bucketDate(event))
                .setParameter("episodeId", event.episodeId())
                .setParameter("tier", tierOf(event))
                .setParameter("duration", durationSec)
                .executeUpdate();
    }

    /**
     * Jarohat holatini rollup'ga qo'shadi.
     *
     * <p>O'rtacha emas, <b>yig'indi va son</b> saqlanadi: shunda rollup'larni
     * bir-biriga qo'shish mumkin bo'ladi (kunlik → haftalik) va o'rtachalarni
     * o'rtachalash xatosiga yo'l qo'yilmaydi.
     *
     * <p>{@code below_15_count} alohida sanaladi, chunki bimodal epizod
     * ("yarmi qiynalmoqda, yarmi umuman emas") sog'lom o'rtacha ortida
     * yashirinib qoladi.
     */
    private void accumulateWound(TelemetryEvent event) {
        TelemetryEvent.WoundState wound = event.wound();
        if (wound == null) {
            return;
        }

        entityManager.createNativeQuery("""
                        insert into wound_balance_daily
                            (bucket_date, episode_id, phase, sample_count,
                             hand_integrity_sum, max_integrity_sum, sabr_sum,
                             deaths_sum, flashbacks_sum, opium_uses_sum,
                             below_15_count, updated_at)
                        values (:bucketDate, :episodeId, :phase, 1,
                                :handIntegrity, :maxIntegrity, :sabr,
                                :deaths, :flashbacks, :opium,
                                :below15, now())
                        on conflict (bucket_date, episode_id, phase)
                        do update set sample_count       = wound_balance_daily.sample_count + 1,
                                      hand_integrity_sum = wound_balance_daily.hand_integrity_sum + :handIntegrity,
                                      max_integrity_sum  = wound_balance_daily.max_integrity_sum + :maxIntegrity,
                                      sabr_sum           = wound_balance_daily.sabr_sum + :sabr,
                                      deaths_sum         = wound_balance_daily.deaths_sum + :deaths,
                                      flashbacks_sum     = wound_balance_daily.flashbacks_sum + :flashbacks,
                                      opium_uses_sum     = wound_balance_daily.opium_uses_sum + :opium,
                                      below_15_count     = wound_balance_daily.below_15_count + :below15,
                                      updated_at         = now()
                        """)
                .setParameter("bucketDate", bucketDate(event))
                .setParameter("episodeId", event.episodeId())
                .setParameter("phase", wound.phase().name())
                .setParameter("handIntegrity", (double) wound.handIntegrity())
                .setParameter("maxIntegrity", (double) wound.maxIntegrity())
                .setParameter("sabr", (double) wound.sabr())
                .setParameter("deaths", (long) wound.deathsThisEpisode())
                .setParameter("flashbacks", (long) wound.flashbacksThisSession())
                .setParameter("opium", (long) wound.opiumUsesTotal())
                .setParameter("below15", wound.handIntegrity() < PUNISHING_INTEGRITY_THRESHOLD ? 1L : 0L)
                .executeUpdate();
    }

    /**
     * Juda qattiq epizodlarni belgilaydi — kunlik job chaqiradi.
     *
     * <p>Chegara 05_MIH_SYSTEM.md §8 dan: o'rtacha {@code hand_integrity < 15}
     * <b>va</b> o'rtacha o'lim {@code > 6}. Ikkalasi birga bo'lishi shart —
     * past integrity o'z-o'zicha yomon emas, u tizimning maqsadi; muammo faqat
     * o'yinchi <i>o'tolmay qolganda</i> boshlanadi.
     *
     * <p>Belgilangan qatorlar live-ops panelida qizil rangda chiqadi va
     * odatda {@code woundDecayScale} ni pasaytirish bilan hal qilinadi.
     *
     * @param bucketDate the day to evaluate
     * @return how many episode/phase rows were flagged
     */
    @Transactional
    public int flagPunishingEpisodes(LocalDate bucketDate) {
        int flagged = entityManager.createNativeQuery("""
                        update wound_balance_daily
                        set flagged_too_punishing = true
                        where bucket_date = :bucketDate
                          and sample_count >= :minSamples
                          and (hand_integrity_sum / sample_count) < :integrityThreshold
                          and (deaths_sum::double precision / sample_count) > :deathsThreshold
                          and flagged_too_punishing = false
                        """)
                .setParameter("bucketDate", bucketDate)
                .setParameter("minSamples", MIN_SAMPLES_FOR_FLAG)
                .setParameter("integrityThreshold", (double) PUNISHING_INTEGRITY_THRESHOLD)
                .setParameter("deathsThreshold", PUNISHING_DEATHS_THRESHOLD)
                .executeUpdate();

        if (flagged > 0) {
            log.warn("""
                    Wound balance: {} episode/phase rows flagged as too punishing on {}. \
                    Live-ops should review woundDecayScale for these episodes.""",
                    flagged, bucketDate);
        }
        return flagged;
    }

    // ── helpers ─────────────────────────────────────────────────────────────

    /**
     * Hodisa qaysi kunga tegishli.
     *
     * <p>UTC bo'yicha, o'yinchi mahalliy vaqti bo'yicha emas: aks holda bir
     * epizodning statistikasi vaqt mintaqalari bo'ylab ikki kunga bo'linib
     * ketadi va kunlik taqqoslash ma'nosini yo'qotadi.
     */
    private static LocalDate bucketDate(TelemetryEvent event) {
        return event.occurredAt().atZone(ZoneOffset.UTC).toLocalDate();
    }

    private static String tierOf(TelemetryEvent event) {
        return event.difficultyTier() == null
                ? DifficultyTier.ALP.name()
                : event.difficultyTier().name();
    }

    private static long parseLongAttribute(TelemetryEvent event, String key) {
        String raw = event.attributes().get(key);
        if (raw == null) {
            return 0L;
        }
        try {
            return Long.parseLong(raw);
        } catch (NumberFormatException e) {
            return 0L;
        }
    }
}
