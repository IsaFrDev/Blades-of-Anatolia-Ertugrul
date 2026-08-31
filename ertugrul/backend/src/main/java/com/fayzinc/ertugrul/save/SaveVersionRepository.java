package com.fayzinc.ertugrul.save;

import org.springframework.data.domain.Limit;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface SaveVersionRepository extends JpaRepository<SaveVersion, UUID> {

    Optional<SaveVersion> findBySlotIdAndVersion(UUID slotId, long version);

    List<SaveVersion> findBySlotIdOrderByVersionDesc(UUID slotId, Limit limit);

    /**
     * Dedupe check: has this exact content already been stored in this slot?
     *
     * <p>Klient tarmoq uzilishidan keyin qayta urinishi odatiy hol. Bir xil
     * blob ikkinchi marta kelsa, yangi versiya yaratish — bekorga joy va
     * bekorga vector clock o'sishi.
     */
    Optional<SaveVersion> findFirstBySlotIdAndSha256OrderByVersionDesc(UUID slotId, String sha256);

    /**
     * The conflict copy a player can restore, if any.
     *
     * <p>Faqat eng oxirgi mag'lub versiya qaytariladi: o'yinchiga bitta aniq
     * tanlov ko'rsatiladi ("boshqa qurilmadagi saqlash"), o'nlab variant emas.
     */
    @Query("""
            select v
            from SaveVersion v
            where v.slot.id = :slotId
              and v.conflictLostTo is not null
            order by v.version desc
            limit 1
            """)
    Optional<SaveVersion> findLatestConflictLoser(@Param("slotId") UUID slotId);

    /**
     * Prunable versions: superseded, not a retained conflict copy, and outside
     * the version tail we keep for support restores.
     *
     * @param slotId the slot to prune
     * @param keepAboveVersion versions at or below this are candidates
     */
    @Query("""
            select v
            from SaveVersion v
            where v.slot.id = :slotId
              and v.supersededAt is not null
              and v.conflictLostTo is null
              and v.version <= :keepAboveVersion
            order by v.version asc
            """)
    List<SaveVersion> findPrunable(@Param("slotId") UUID slotId,
                                   @Param("keepAboveVersion") long keepAboveVersion);

    /** Every object key a player owns — GDPR erasure needs the full list. */
    @Query("""
            select v.objectKey
            from SaveVersion v
            where v.slot.player.id = :playerId
            """)
    List<String> findAllObjectKeysForPlayer(@Param("playerId") UUID playerId);

    /** Highest version number issued in a slot, or 0 when empty. */
    @Query("""
            select coalesce(max(v.version), 0)
            from SaveVersion v
            where v.slot.id = :slotId
            """)
    long findMaxVersion(@Param("slotId") UUID slotId);
}
