package com.fayzinc.ertugrul.identity;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Modifying;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface RefreshTokenRepository extends JpaRepository<RefreshToken, UUID> {

    Optional<RefreshToken> findByTokenHash(String tokenHash);

    /**
     * Revokes an entire rotation family.
     *
     * <p>Ishlatilgan token qayta kelganda chaqiriladi. Bu — o'g'irlikka javob:
     * qaysi nusxa haqiqiy egasiniki ekanini ajratib bo'lmaydi, shuning uchun
     * ikkalasi ham bekor qilinadi va o'yinchi qayta kiradi. Bir marta noqulaylik
     * — o'g'irlangan sessiyaning cheksiz davom etishidan afzal.
     *
     * @return number of tokens revoked
     */
    @Modifying(clearAutomatically = true)
    @Query("""
            update RefreshToken t
            set t.revokedAt = :now, t.revokedReason = :reason
            where t.familyId = :familyId
              and t.revokedAt is null
            """)
    int revokeFamily(@Param("familyId") UUID familyId,
                     @Param("now") Instant now,
                     @Param("reason") String reason);

    /** Revokes every live session for a player — used on logout-all and erasure. */
    @Modifying(clearAutomatically = true)
    @Query("""
            update RefreshToken t
            set t.revokedAt = :now, t.revokedReason = :reason
            where t.player.id = :playerId
              and t.revokedAt is null
            """)
    int revokeAllForPlayer(@Param("playerId") UUID playerId,
                           @Param("now") Instant now,
                           @Param("reason") String reason);

    /**
     * Housekeeping: delete tokens that expired long ago.
     *
     * <p>Bekor qilingan token'lar darhol o'chirilmaydi — qayta ishlatish
     * urinishini aniqlash uchun ular biroz vaqt turishi kerak.
     */
    @Modifying
    @Query("delete from RefreshToken t where t.expiresAt < :cutoff")
    int deleteExpiredBefore(@Param("cutoff") Instant cutoff);
}
