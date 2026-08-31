package com.fayzinc.ertugrul.journey;

import org.springframework.data.domain.Limit;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

/**
 * Journey Log repositories.
 *
 * <p>Uchtasi bitta faylda: ular bitta xususiyatning bo'laklari va doim birga
 * o'zgaradi.
 */
public final class JourneyRepository {

    private JourneyRepository() {
    }

    @Repository
    public interface JourneyEntryRepository extends JpaRepository<JourneyEntry, UUID> {

        List<JourneyEntry> findByPlayerIdAndPlaythroughIdOrderBySequenceNoAsc(
                UUID playerId, UUID playthroughId);

        Optional<JourneyEntry> findByPlaythroughIdAndSequenceNo(UUID playthroughId, int sequenceNo);

        /**
         * Pages inside a shared range, excluding anything the player hid.
         *
         * <p>Ommaviy sahifa uchun. {@code hiddenFromShare} filtri bu yerda,
         * so'rovning o'zida — Java tomonida filtrlash yashirin sahifani
         * tasodifan chiqarib yuborish uchun juda oson yo'l bo'lardi.
         */
        @Query("""
                select e
                from JourneyEntry e
                where e.playthroughId = :playthroughId
                  and e.hiddenFromShare = false
                  and (:fromSeq is null or e.sequenceNo >= :fromSeq)
                  and (:toSeq is null or e.sequenceNo <= :toSeq)
                order by e.sequenceNo asc
                """)
        List<JourneyEntry> findShareable(@Param("playthroughId") UUID playthroughId,
                                         @Param("fromSeq") Integer fromSeq,
                                         @Param("toSeq") Integer toSeq);

        List<JourneyEntry> findByPlayerIdOrderByCreatedAtDesc(UUID playerId, Limit limit);

        long countByPlayerId(UUID playerId);

        /** Distinct playthroughs, newest first — the "which diary?" picker. */
        @Query("""
                select e.playthroughId, min(e.createdAt), max(e.sequenceNo), count(e)
                from JourneyEntry e
                where e.player.id = :playerId
                group by e.playthroughId
                order by min(e.createdAt) desc
                """)
        List<Object[]> summarisePlaythroughs(@Param("playerId") UUID playerId);
    }

    @Repository
    public interface JourneyShareRepository extends JpaRepository<JourneyShare, UUID> {

        Optional<JourneyShare> findByShareToken(String shareToken);

        List<JourneyShare> findByPlayerId(UUID playerId);

        /** Live links for one playthrough — the client shows these in the share sheet. */
        @Query("""
                select s
                from JourneyShare s
                where s.player.id = :playerId
                  and s.playthroughId = :playthroughId
                  and s.revokedAt is null
                  and s.expiresAt > :now
                """)
        List<JourneyShare> findLiveForPlaythrough(@Param("playerId") UUID playerId,
                                                  @Param("playthroughId") UUID playthroughId,
                                                  @Param("now") Instant now);
    }

    @Repository
    public interface JourneyExportRepository extends JpaRepository<JourneyExport, UUID> {

        List<JourneyExport> findByPlayerIdOrderByRequestedAtDesc(UUID playerId, Limit limit);

        /**
         * Worklist for the export worker.
         *
         * <p>Ikki xil ish olinadi va ular <b>bir xil shart bilan emas</b>:
         * <ul>
         *   <li>{@code PENDING} — darhol, hech qanday kutishsiz. O'yinchi
         *       eksport so'ragan, uni 5 daqiqa kutdirish ma'nosiz;</li>
         *   <li>{@code RUNNING} — faqat {@code staleBefore} dan eski bo'lsa.
         *       Bu worker generatsiya paytida yiqilgan degani; aks holda ish
         *       hozir ham bajarilayotgan bo'lishi mumkin va uni ikkinchi marta
         *       boshlash ikki xil natija yozishga olib keladi.</li>
         * </ul>
         */
        @Query("""
                select e
                from JourneyExport e
                where (e.status = :pending)
                   or (e.status = :running and e.requestedAt < :staleBefore)
                order by e.requestedAt asc
                """)
        List<JourneyExport> findClaimable(@Param("pending") JourneyExport.Status pending,
                                          @Param("running") JourneyExport.Status running,
                                          @Param("staleBefore") Instant staleBefore,
                                          Limit limit);

        /** Expired but still marked READY — their objects need deleting. */
        List<JourneyExport> findByStatusAndExpiresAtBefore(
                JourneyExport.Status status, Instant now, Limit limit);
    }
}
