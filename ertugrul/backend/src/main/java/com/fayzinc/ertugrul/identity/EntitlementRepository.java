package com.fayzinc.ertugrul.identity;

import org.springframework.data.domain.Limit;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface EntitlementRepository extends JpaRepository<Entitlement, UUID> {

    Optional<Entitlement> findByProviderAndProviderAccountIdAndProductSku(
            Entitlement.Provider provider, String providerAccountId, String productSku);

    List<Entitlement> findByPlayerId(UUID playerId);

    boolean existsByPlayerIdAndStatus(UUID playerId, Entitlement.Status status);

    /**
     * Worklist for the periodic revalidation job.
     *
     * <p>Refund va chargeback entitlement'ni <i>keyin</i> bekor qiladi, shuning
     * uchun bir marta tekshirish yetarli emas.
     */
    List<Entitlement> findByStatusAndRevalidateAfterBefore(
            Entitlement.Status status, Instant now, Limit limit);
}
