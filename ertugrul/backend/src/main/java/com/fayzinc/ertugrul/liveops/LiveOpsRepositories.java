package com.fayzinc.ertugrul.liveops;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

/**
 * Live-ops repositories.
 *
 * <p>Uchtasi bitta faylda: ular juda kichik, bir xil hayot davriga ega va
 * har doim birga o'qiladi. Har birini alohida faylga ajratish bu yerda
 * navigatsiyani osonlashtirmaydi, faqat fayl sonini oshiradi.
 */
public final class LiveOpsRepositories {

    private LiveOpsRepositories() {
    }

    @Repository
    public interface RemoteConfigRepository extends JpaRepository<RemoteConfig, UUID> {

        Optional<RemoteConfig> findByConfigKey(String configKey);

        /**
         * Every config inside its activation window.
         *
         * <p>Kogort filtri bu yerda emas — u Java tomonida qo'llaniladi, chunki
         * kogort raqami o'yinchiga bog'liq va uni SQL'ga uzatish har o'yinchi
         * uchun alohida rejalashtirilgan so'rov degani bo'lardi. Faol
         * konfiguratsiyalar soni o'nlab, shuning uchun hammasini o'qib, keshda
         * saqlash arzonroq.
         */
        @Query("""
                select c
                from RemoteConfig c
                where c.activeFrom <= :now
                  and (c.activeUntil is null or c.activeUntil > :now)
                """)
        List<RemoteConfig> findActive(@Param("now") Instant now);

        /** Highest revision across all active configs — the ETag input. */
        @Query("""
                select coalesce(max(c.revision), 0)
                from RemoteConfig c
                where c.activeFrom <= :now
                  and (c.activeUntil is null or c.activeUntil > :now)
                """)
        long maxActiveRevision(@Param("now") Instant now);
    }

    @Repository
    public interface EpisodeBalanceOverrideRepository
            extends JpaRepository<EpisodeBalanceOverride, UUID> {

        List<EpisodeBalanceOverride> findByEpisodeIdAndEnabledTrue(String episodeId);

        Optional<EpisodeBalanceOverride> findByEpisodeIdAndVariantKey(String episodeId, String variantKey);

        List<EpisodeBalanceOverride> findByEnabledTrue();

        /** Highest revision, used as the balance document's ETag. */
        @Query("""
                select coalesce(max(o.revision), 0)
                from EpisodeBalanceOverride o
                where o.enabled = true
                """)
        long maxEnabledRevision();
    }

    @Repository
    public interface SeasonalEventRepository extends JpaRepository<SeasonalEvent, UUID> {

        Optional<SeasonalEvent> findByEventKey(String eventKey);

        /** Events running right now — the client polls this on the main menu. */
        @Query("""
                select e
                from SeasonalEvent e
                where e.enabled = true
                  and e.startsAt <= :now
                  and e.endsAt > :now
                order by e.startsAt asc
                """)
        List<SeasonalEvent> findLive(@Param("now") Instant now);
    }
}
