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
 * A machine the player plays on.
 *
 * <p><b>Nega bu alohida jadval.</b> Qurilma identifikatori shunchaki analitika
 * yorlig'i emas — u <b>vector clock'ning tugun (node) kaliti</b>. Bulutli
 * saqlashdagi konflikt hal qilish to'g'ridan-to'g'ri shu qatorga tayanadi:
 * "PC EP023 gacha o'ynadi, PS5 esa offline EP021 da qoldi" degan holatni
 * faqat qurilmalar ro'yxati aniq bo'lsagina ajratish mumkin.
 *
 * @see com.fayzinc.ertugrul.save.ConflictResolver
 */
@Entity
@Table(name = "player_device")
public class PlayerDevice {

    public enum Platform {
        PC_STEAM,
        PC_EPIC,
        PS5,
        XBOX_SERIES,
        UNKNOWN
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false)
    private Player player;

    /**
     * Client-generated, stable across reinstalls where the platform allows it.
     * Globally unique — this is the vector-clock node key, and two players
     * sharing one would corrupt each other's merge.
     */
    @Column(name = "device_id", nullable = false, length = 64, updatable = false)
    private String deviceId;

    @Enumerated(EnumType.STRING)
    @Column(name = "platform", nullable = false, length = 16)
    private Platform platform = Platform.UNKNOWN;

    @Column(name = "device_label", length = 64)
    private String deviceLabel;

    @Column(name = "app_version", length = 32)
    private String appVersion;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    @Column(name = "last_seen_at", nullable = false)
    private Instant lastSeenAt = Instant.now();

    protected PlayerDevice() {
        // JPA
    }

    public PlayerDevice(Player player, String deviceId, Platform platform, String appVersion) {
        this.player = player;
        this.deviceId = deviceId;
        this.platform = platform == null ? Platform.UNKNOWN : platform;
        this.appVersion = appVersion;
    }

    public void touch(String appVersion) {
        this.lastSeenAt = Instant.now();
        if (appVersion != null && !appVersion.isBlank()) {
            this.appVersion = appVersion;
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

    public Platform getPlatform() {
        return platform;
    }

    public String getDeviceLabel() {
        return deviceLabel;
    }

    public void setDeviceLabel(String deviceLabel) {
        this.deviceLabel = deviceLabel;
    }

    public String getAppVersion() {
        return appVersion;
    }

    public Instant getLastSeenAt() {
        return lastSeenAt;
    }

    public Instant getCreatedAt() {
        return createdAt;
    }
}
