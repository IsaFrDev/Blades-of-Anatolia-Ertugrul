package com.fayzinc.ertugrul.identity;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.Table;

import java.time.Instant;
import java.util.UUID;

/**
 * Proof that the player owns the game on a given store.
 *
 * <p><b>Nima uchun kerak.</b> Bu DRM emas — o'yin offline ishlaydi va
 * entitlement tekshiruvi o'yinni bloklamaydi. U ikki narsa uchun kerak:
 * (1) anonim qurilma akkauntini <i>tiklanadigan</i> qilish — o'yinchi
 * konsolini yo'qotsa, Steam akkaunti orqali progressini qaytaradi;
 * (2) bulut resurslaridan (save, PDF eksport) suiiste'foldan himoya.
 *
 * <p>Refund va chargeback'lar entitlement'ni <i>keyin</i> bekor qiladi,
 * shuning uchun {@code revalidateAfter} orqali davriy qayta tekshiriladi.
 */
@Entity
@Table(name = "entitlement")
public class Entitlement {

    public enum Provider {
        STEAM,
        PSN,
        XBOX,
        EPIC
    }

    public enum Status {
        VALID,
        REVOKED,
        REFUNDED,
        /** Platform SDK unreachable at link time; retried by the revalidation job. */
        UNVERIFIED
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false)
    private Player player;

    @Enumerated(EnumType.STRING)
    @Column(name = "provider", nullable = false, length = 16, updatable = false)
    private Provider provider;

    /** SteamID64 / PSN account id / XUID. */
    @Column(name = "provider_account_id", nullable = false, length = 64, updatable = false)
    private String providerAccountId;

    @Column(name = "product_sku", nullable = false, length = 64)
    private String productSku = "DIRILIS_BASE";

    @Enumerated(EnumType.STRING)
    @Column(name = "status", nullable = false, length = 16)
    private Status status = Status.UNVERIFIED;

    @Column(name = "verified_at")
    private Instant verifiedAt;

    @Column(name = "revalidate_after")
    private Instant revalidateAfter;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    protected Entitlement() {
        // JPA
    }

    public Entitlement(Player player, Provider provider, String providerAccountId, String productSku) {
        this.player = player;
        this.provider = provider;
        this.providerAccountId = providerAccountId;
        if (productSku != null && !productSku.isBlank()) {
            this.productSku = productSku;
        }
    }

    /**
     * Marks the entitlement verified and schedules the next revalidation.
     *
     * @param nextCheck when to re-ask the platform; refunds arrive after the fact
     */
    public void markVerified(Instant nextCheck) {
        this.status = Status.VALID;
        this.verifiedAt = Instant.now();
        this.revalidateAfter = nextCheck;
    }

    public void markInvalid(Status reason) {
        this.status = reason;
        this.revalidateAfter = null;
    }

    public boolean isValid() {
        return status == Status.VALID;
    }

    public UUID getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public Provider getProvider() {
        return provider;
    }

    public String getProviderAccountId() {
        return providerAccountId;
    }

    public String getProductSku() {
        return productSku;
    }

    public Status getStatus() {
        return status;
    }

    public Instant getVerifiedAt() {
        return verifiedAt;
    }

    public Instant getRevalidateAfter() {
        return revalidateAfter;
    }
}
