package com.fayzinc.ertugrul.journey;

import com.fayzinc.ertugrul.identity.Player;
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
 * A requested PDF/PNG export of the diary.
 *
 * <p><b>Nega asinxron.</b> 48 epizodlik to'liq daftar — bu haqiqiy hujjat,
 * o'nlab sahifa. Uni so'rov ichida generatsiya qilish HTTP thread'ini
 * sekundlab band qiladi va timeout xavfini tug'diradi. Shuning uchun:
 * so'rov {@code PENDING} qator yaratadi, worker uni bajaradi, klient esa
 * holatini so'rab turadi.
 */
@Entity
@Table(name = "journey_export")
public class JourneyExport {

    public enum Format {
        PDF,
        /** A single tall image, for social platforms that dislike PDFs. */
        PNG
    }

    public enum Status {
        PENDING,
        RUNNING,
        READY,
        FAILED,
        /** Cleaned up after ertugrul.journey.export-ttl. */
        EXPIRED
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false, updatable = false)
    private Player player;

    @Column(name = "playthrough_id", nullable = false, updatable = false)
    private UUID playthroughId;

    @Enumerated(EnumType.STRING)
    @Column(name = "format", nullable = false, length = 8)
    private Format format = Format.PDF;

    @Enumerated(EnumType.STRING)
    @Column(name = "status", nullable = false, length = 16)
    private Status status = Status.PENDING;

    @Column(name = "object_key", length = 256)
    private String objectKey;

    @Column(name = "size_bytes")
    private Integer sizeBytes;

    @Column(name = "entry_count")
    private Integer entryCount;

    @Column(name = "failure_reason", length = 256)
    private String failureReason;

    @Column(name = "requested_at", nullable = false, updatable = false)
    private Instant requestedAt = Instant.now();

    @Column(name = "completed_at")
    private Instant completedAt;

    @Column(name = "expires_at", nullable = false)
    private Instant expiresAt;

    protected JourneyExport() {
        // JPA
    }

    public JourneyExport(Player player, UUID playthroughId, Format format, Instant expiresAt) {
        this.player = player;
        this.playthroughId = playthroughId;
        this.format = format == null ? Format.PDF : format;
        this.expiresAt = expiresAt;
    }

    public void markRunning() {
        this.status = Status.RUNNING;
    }

    public void markReady(String objectKey, int sizeBytes, int entryCount) {
        this.status = Status.READY;
        this.objectKey = objectKey;
        this.sizeBytes = sizeBytes;
        this.entryCount = entryCount;
        this.completedAt = Instant.now();
    }

    /**
     * Marks the export failed.
     *
     * <p>Sabab qisqartiriladi: u ustunga sig'ishi va log'da o'qilishi kerak,
     * lekin klientga to'liq stack trace hech qachon ko'rsatilmaydi.
     */
    public void markFailed(String reason) {
        this.status = Status.FAILED;
        this.failureReason = reason == null ? "unknown"
                : reason.substring(0, Math.min(reason.length(), 256));
        this.completedAt = Instant.now();
    }

    public boolean isReady() {
        return status == Status.READY;
    }

    public UUID getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public UUID getPlaythroughId() {
        return playthroughId;
    }

    public Format getFormat() {
        return format;
    }

    public Status getStatus() {
        return status;
    }

    public String getObjectKey() {
        return objectKey;
    }

    public Integer getSizeBytes() {
        return sizeBytes;
    }

    public Integer getEntryCount() {
        return entryCount;
    }

    public String getFailureReason() {
        return failureReason;
    }

    public Instant getRequestedAt() {
        return requestedAt;
    }

    public Instant getCompletedAt() {
        return completedAt;
    }

    public Instant getExpiresAt() {
        return expiresAt;
    }
}
