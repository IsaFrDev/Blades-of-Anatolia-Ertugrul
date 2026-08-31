package com.fayzinc.ertugrul.save;

import jakarta.persistence.LockModeType;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Lock;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface SaveRepository extends JpaRepository<SaveSlot, UUID> {

    List<SaveSlot> findByPlayerIdOrderBySlotIndexAsc(UUID playerId);

    Optional<SaveSlot> findByPlayerIdAndSlotIndex(UUID playerId, short slotIndex);

    /**
     * Slotni yozish uchun qulflab o'qiydi.
     *
     * <p><b>Nega pessimistic lock.</b> Yuklash oqimi "boshni o'qi → konfliktni
     * hal qil → boshni siljit" ketma-ketligidan iborat. Ikki qurilma bir vaqtda
     * yozsa, optimistic lock ikkalasini ham o'qishga qo'yib, keyin birini rad
     * etadi — va o'sha rad etilgan urinish allaqachon S3'ga blob yozib
     * bo'lgan bo'ladi (yetim obyekt). Qulf bilan esa ikkinchi so'rov
     * <i>yangilangan</i> boshni ko'radi va konfliktni to'g'ri aniqlaydi.
     *
     * <p>Qulf faqat bitta qator ustida va tranzaksiya juda qisqa: blob S3'ga
     * tranzaksiyadan <b>tashqarida</b> yoziladi.
     */
    @Lock(LockModeType.PESSIMISTIC_WRITE)
    @Query("""
            select s
            from SaveSlot s
            where s.player.id = :playerId
              and s.slotIndex = :slotIndex
            """)
    Optional<SaveSlot> findForUpdate(@Param("playerId") UUID playerId,
                                     @Param("slotIndex") short slotIndex);

    /** Slots with an unresolved conflict copy — drives the client's "restore?" prompt. */
    List<SaveSlot> findByPlayerIdAndHasConflictTrue(UUID playerId);

    boolean existsByPlayerIdAndSlotIndex(UUID playerId, short slotIndex);

    /** Total bytes a player occupies; used for quota reporting and abuse detection. */
    @Query("""
            select coalesce(sum(v.sizeBytes), 0)
            from SaveVersion v
            where v.slot.player.id = :playerId
            """)
    long totalBytesForPlayer(@Param("playerId") UUID playerId);
}
