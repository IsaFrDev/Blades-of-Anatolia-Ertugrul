package com.fayzinc.ertugrul.codex;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface CodexRepository extends JpaRepository<CodexEntryProgress, UUID> {

    List<CodexEntryProgress> findByPlayerId(UUID playerId);

    Optional<CodexEntryProgress> findByPlayerIdAndCodexId(UUID playerId, String codexId);

    /**
     * Batch load for a sync round.
     *
     * <p>Bir necha o'nlab yozuv kelganda har biriga alohida so'rov yubormaslik
     * uchun — bu N+1 muammosining eng oddiy ko'rinishi va sinxronni sekundlarga
     * cho'zadi.
     */
    List<CodexEntryProgress> findByPlayerIdAndCodexIdIn(UUID playerId, Collection<String> codexIds);

    /**
     * Delta sync: only what changed since the client's last successful pull.
     *
     * <p>To'liq ro'yxat 180 ta yozuv — ko'p emas, lekin har sessiyada uni
     * tashish keraksiz. Klient oxirgi sinxron vaqtini saqlaydi va faqat
     * farqni so'raydi.
     */
    List<CodexEntryProgress> findByPlayerIdAndUpdatedAtAfter(UUID playerId, Instant since);

    long countByPlayerId(UUID playerId);

    /** Collection progress per category, for the codex screen's completion bars. */
    @Query("""
            select c.category, count(c)
            from CodexEntryProgress c
            where c.player.id = :playerId
            group by c.category
            """)
    List<Object[]> countByCategoryForPlayer(@Param("playerId") UUID playerId);

    /**
     * Unlocked-but-unread count, split by confidence.
     *
     * <p>Bu — tarix qatlamining <b>samaradorlik o'lchovi</b>. Agar
     * {@code DISPUTED} yozuvlar {@code DOCUMENTED} laridan sezilarli
     * darajada kam o'qilsa, demak "bahsli" belgisi o'yinchini qiziqtirish
     * o'rniga qo'rqitayapti — va bu butun konsepsiyani qayta ko'rib chiqish
     * signali.
     */
    @Query("""
            select c.confidence, count(c)
            from CodexEntryProgress c
            where c.player.id = :playerId
              and c.readCount = 0
            group by c.confidence
            """)
    List<Object[]> countUnreadByConfidence(@Param("playerId") UUID playerId);

    /** Global rollup: which entries players unlock but never open. */
    @Query("""
            select c.codexId, count(c), sum(case when c.readCount = 0 then 1 else 0 end)
            from CodexEntryProgress c
            group by c.codexId
            order by count(c) desc
            """)
    List<Object[]> globalUnlockVersusReadCounts();
}
