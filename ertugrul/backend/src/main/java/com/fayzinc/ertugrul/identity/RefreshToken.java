package com.fayzinc.ertugrul.identity;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.Table;

import java.time.Instant;
import java.util.UUID;

/**
 * A stored, hashed refresh token.
 *
 * <p><b>Rotatsiya va o'g'irlikni aniqlash.</b> Har bir refresh yangi token
 * beradi va eskisini <i>ishlatilgan</i> deb belgilaydi. Agar allaqachon
 * ishlatilgan token yana kelsa — bu ikki narsani anglatadi: yo token o'g'irlangan,
 * yo klient javobni olmay qayta urinmoqda. Ikkalasini ham ajratib bo'lmagani
 * uchun butun <i>oila</i> (family) bekor qilinadi va o'yinchi qayta kiradi.
 *
 * <p>Token'ning o'zi hech qachon saqlanmaydi — faqat SHA-256 hash'i. Bazaga
 * kirgan hujumchi sessiya yarata olmaydi.
 */
@Entity
@Table(name = "refresh_token")
public class RefreshToken {

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false)
    private Player player;

    @Column(name = "device_id", nullable = false, length = 64)
    private String deviceId;

    /** SHA-256 hex of the opaque token. */
    @Column(name = "token_hash", nullable = false, length = 64, updatable = false)
    private String tokenHash;

    /** All descendants of a single login share this id. */
    @Column(name = "family_id", nullable = false, updatable = false)
    private UUID familyId;

    @Column(name = "issued_at", nullable = false, updatable = false)
    private Instant issuedAt = Instant.now();

    @Column(name = "expires_at", nullable = false)
    private Instant expiresAt;

    @Column(name = "used_at")
    private Instant usedAt;

    @Column(name = "revoked_at")
    private Instant revokedAt;

    @Column(name = "revoked_reason", length = 48)
    private String revokedReason;

    protected RefreshToken() {
        // JPA
    }

    public RefreshToken(Player player, String deviceId, String tokenHash, UUID familyId, Instant expiresAt) {
        this.player = player;
        this.deviceId = deviceId;
        this.tokenHash = tokenHash;
        this.familyId = familyId;
        this.expiresAt = expiresAt;
    }

    /** Usable exactly once, while unexpired and unrevoked. */
    public boolean isUsable(Instant now) {
        return usedAt == null && revokedAt == null && now.isBefore(expiresAt);
    }

    public boolean isAlreadyUsed() {
        return usedAt != null;
    }

    public void markUsed() {
        this.usedAt = Instant.now();
    }

    public void revoke(String reason) {
        if (this.revokedAt == null) {
            this.revokedAt = Instant.now();
            this.revokedReason = reason;
        }
    }

    public UUID getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public String getDeviceId() {
        return deviceId;
    }

    public String getTokenHash() {
        return tokenHash;
    }

    public UUID getFamilyId() {
        return familyId;
    }

    public Instant getExpiresAt() {
        return expiresAt;
    }

    public Instant getUsedAt() {
        return usedAt;
    }

    public Instant getRevokedAt() {
        return revokedAt;
    }

    public String getRevokedReason() {
        return revokedReason;
    }
}
