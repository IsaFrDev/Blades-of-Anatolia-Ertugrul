package com.fayzinc.ertugrul.save;

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
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.time.Instant;
import java.util.Map;
import java.util.UUID;

/**
 * One immutable uploaded save.
 *
 * <p><b>Nega o'zgarmas (immutable) tarix.</b> Bu single-player o'yin: 40
 * daqiqalik progressni jimgina o'chirib yuborish — kechirilmas xato. Shuning
 * uchun har yuklash yangi versiya bo'lib yoziladi, eskisi esa saqlanadi.
 * Konfliktda mag'lub bo'lgan tomon ham o'chirilmaydi — u
 * {@link #conflictLostTo} orqali g'olibga bog'lanadi va o'yinchi (yoki
 * qo'llab-quvvatlash) uni qaytara oladi.
 */
@Entity
@Table(name = "save_version")
public class SaveVersion {

    /** How this version came to be, for support triage and metrics. */
    public enum Resolution {
        /** First ever save in the slot. */
        INITIAL,
        /** Incoming clock strictly dominated the head: a clean fast-forward. */
        FAST_FORWARD,
        /** Concurrent clocks; this version won the last-write-wins tiebreak. */
        LWW_WINNER,
        /** Concurrent clocks; this version lost but is retained and restorable. */
        LWW_LOSER,
        /** Identical content re-uploaded; stored as a no-op pointer. */
        IDENTICAL
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "slot_id", nullable = false, updatable = false)
    private SaveSlot slot;

    @Column(name = "version", nullable = false, updatable = false)
    private long version;

    /** {@code saves/{playerId}/{slotIndex}/{version}.sav} */
    @Column(name = "object_key", nullable = false, length = 256, updatable = false)
    private String objectKey;

    @Column(name = "size_bytes", nullable = false, updatable = false)
    private int sizeBytes;

    /** Content hash — enables dedupe and feeds the server HMAC. */
    @Column(name = "sha256", nullable = false, length = 64, updatable = false)
    private String sha256;

    /** HMAC over (playerId | slotIndex | version | sha256), server key. */
    @Column(name = "server_hmac", nullable = false, length = 64, updatable = false)
    private String serverHmac;

    /** Client HMAC verification result; {@code null} when the client sent none. */
    @Column(name = "client_hmac_valid")
    private Boolean clientHmacValid;

    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "vector_clock", nullable = false, columnDefinition = "jsonb", updatable = false)
    private Map<String, Long> vectorClock = Map.of();

    @Column(name = "origin_device_id", nullable = false, length = 64, updatable = false)
    private String originDeviceId;

    /**
     * When the CLIENT believes it saved.
     *
     * <p>Ishonchsiz: bu faqat LWW uchun teng holatda hakam. Ruxsat etilgan
     * soat farqidan oshsa, yuklash butunlay rad etiladi.
     */
    @Column(name = "client_saved_at", nullable = false, updatable = false)
    private Instant clientSavedAt;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "conflict_lost_to")
    private SaveVersion conflictLostTo;

    @Enumerated(EnumType.STRING)
    @Column(name = "resolution", length = 24)
    private Resolution resolution;

    @Column(name = "superseded_at")
    private Instant supersededAt;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    protected SaveVersion() {
        // JPA
    }

    public SaveVersion(SaveSlot slot, long version, String objectKey, int sizeBytes,
                       String sha256, String serverHmac, VectorClock clock,
                       String originDeviceId, Instant clientSavedAt) {
        this.slot = slot;
        this.version = version;
        this.objectKey = objectKey;
        this.sizeBytes = sizeBytes;
        this.sha256 = sha256;
        this.serverHmac = serverHmac;
        this.vectorClock = clock.counters();
        this.originDeviceId = originDeviceId;
        this.clientSavedAt = clientSavedAt;
    }

    public VectorClock clock() {
        return VectorClock.of(vectorClock);
    }

    /** Marks this version as no longer the head. The blob is kept. */
    public void supersede() {
        if (this.supersededAt == null) {
            this.supersededAt = Instant.now();
        }
    }

    /**
     * Records that this version lost a last-write-wins tiebreak.
     *
     * <p>Blob saqlanib qoladi — o'yinchiga "boshqa qurilmadagi saqlashni
     * tiklash" imkoniyati beriladi.
     */
    public void loseTo(SaveVersion winner) {
        this.conflictLostTo = winner;
        this.resolution = Resolution.LWW_LOSER;
        supersede();
    }

    public void setResolution(Resolution resolution) {
        this.resolution = resolution;
    }

    public void setClientHmacValid(Boolean clientHmacValid) {
        this.clientHmacValid = clientHmacValid;
    }

    // ── Accessors ───────────────────────────────────────────────────────────

    public UUID getId() {
        return id;
    }

    public SaveSlot getSlot() {
        return slot;
    }

    public long getVersion() {
        return version;
    }

    public String getObjectKey() {
        return objectKey;
    }

    public int getSizeBytes() {
        return sizeBytes;
    }

    public String getSha256() {
        return sha256;
    }

    public String getServerHmac() {
        return serverHmac;
    }

    public Boolean getClientHmacValid() {
        return clientHmacValid;
    }

    public Map<String, Long> getVectorClock() {
        return vectorClock;
    }

    public String getOriginDeviceId() {
        return originDeviceId;
    }

    public Instant getClientSavedAt() {
        return clientSavedAt;
    }

    public SaveVersion getConflictLostTo() {
        return conflictLostTo;
    }

    public Resolution getResolution() {
        return resolution;
    }

    public Instant getSupersededAt() {
        return supersededAt;
    }

    public Instant getCreatedAt() {
        return createdAt;
    }
}
