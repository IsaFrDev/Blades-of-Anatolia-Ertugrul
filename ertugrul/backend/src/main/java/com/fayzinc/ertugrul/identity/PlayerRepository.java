package com.fayzinc.ertugrul.identity;

import jakarta.persistence.LockModeType;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Lock;
import org.springframework.data.jpa.repository.Modifying;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface PlayerRepository extends JpaRepository<Player, UUID> {

    /**
     * Resolves the player behind a device id — the hot path of silent login.
     *
     * <p>Join qilinmoqda, chunki qurilma identifikatori global unikal va aynan
     * shu bitta so'rov "O'ynash" tugmasidan keyingi butun autentifikatsiyani
     * hal qiladi.
     */
    @Query("""
            select d.player
            from PlayerDevice d
            where d.deviceId = :deviceId
            """)
    Optional<Player> findByDeviceId(@Param("deviceId") String deviceId);

    /**
     * Resolves a player through a linked store account (account recovery).
     *
     * <p>Status is passed as a bound parameter rather than written as a JPQL
     * enum literal — literals here are a portability trap and read worse.
     */
    @Query("""
            select e.player
            from Entitlement e
            where e.provider = :provider
              and e.providerAccountId = :providerAccountId
              and e.status = :status
            """)
    Optional<Player> findByEntitlement(@Param("provider") Entitlement.Provider provider,
                                       @Param("providerAccountId") String providerAccountId,
                                       @Param("status") Entitlement.Status status);

    /**
     * Pessimistic read for anti-cheat score updates.
     *
     * <p>Telemetriya va save yo'llari bir vaqtda ballni oshirishi mumkin;
     * optimistic lock bu yerda mos emas, chunki konflikt <i>kutilyapti</i> va
     * qayta urinish so'rovni sekinlashtiradi.
     */
    @Lock(LockModeType.PESSIMISTIC_WRITE)
    @Query("select p from Player p where p.id = :id")
    Optional<Player> findByIdForUpdate(@Param("id") UUID id);

    /**
     * Bulk last-seen refresh.
     *
     * <p>Har so'rovda {@code last_seen_at} ni yangilash — bu har so'rovga bitta
     * qo'shimcha yozish degani. O'rniga u navbatga to'planadi va davriy ravishda
     * bitta so'rov bilan yoziladi. Aniqlik bu yerda muhim emas.
     */
    @Modifying(clearAutomatically = true, flushAutomatically = true)
    @Query("""
            update Player p
            set p.lastSeenAt = :seenAt
            where p.id in :ids
            """)
    int touchLastSeen(@Param("ids") List<UUID> ids, @Param("seenAt") Instant seenAt);

    /** GDPR erasure worklist. */
    List<Player> findByStatusAndErasureRequestedAtBefore(Player.PlayerStatus status, Instant cutoff);
}
