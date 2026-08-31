package com.fayzinc.ertugrul.identity;

import com.fayzinc.ertugrul.common.Auditable;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import jakarta.persistence.Version;

import java.time.Instant;
import java.util.Objects;
import java.util.UUID;

/**
 * A human who plays the game.
 *
 * <p><b>Akkaunt falsafasi.</b> O'yin single-player, shuning uchun akkaunt
 * <i>kirish to'sig'i</i> emas. O'yinchi "O'ynash" tugmasini bosadi va hech
 * narsa yozmasdan qurilmaga bog'langan anonim akkaunt oladi. Do'kon
 * entitlement'i (Steam/PSN/Xbox) keyinroq bog'lanadi va aynan u akkauntni
 * <i>tiklanadigan</i> qiladi. Bu tartib ataylab: ro'yxatdan o'tish oynasi
 * birinchi 60 soniyada o'yinchini yo'qotadigan eng keng tarqalgan sabab.
 */
@Entity
@Table(name = "player")
public class Player extends Auditable {

    /**
     * Telemetry consent, from Settings → Account & Cloud → Telemetry.
     * GDPR-relevant: {@link #OFF} means events are dropped at the ingest edge,
     * not merely filtered downstream.
     */
    public enum TelemetryConsent {
        /** Everything, tied to the player id. */
        FULL,
        /** Events keep their shape but the player id is replaced by a rotating pseudonym. */
        ANONYMOUS,
        /** Nothing is produced to Kafka at all. */
        OFF
    }

    public enum PlayerStatus {
        ACTIVE,
        SUSPENDED,
        /** GDPR erasure requested; the nightly job will scrub PII. */
        ERASURE_PENDING,
        ERASED
    }

    /**
     * Above this score a player stops contributing to {@code ChoiceAggregate}.
     *
     * <p>Muhim: bu <b>ban emas</b>. O'yin single-player — kimdir o'z save'ini
     * o'zgartirsa, u faqat o'ziga ta'sir qiladi va bu uning haqqi. Yagona
     * haqiqiy zarar — buzilgan ma'lumot global statistikani ("62% Titusni
     * kechirdi") ifloslantirishi. Shuning uchun jazo bitta: ovozi sanalmaydi.
     */
    public static final int STATS_EXCLUSION_THRESHOLD = 50;

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @Column(name = "display_name", length = 48)
    private String displayName;

    @Enumerated(EnumType.STRING)
    @Column(name = "telemetry_consent", nullable = false, length = 16)
    private TelemetryConsent telemetryConsent = TelemetryConsent.FULL;

    @Column(name = "cloud_save_enabled", nullable = false)
    private boolean cloudSaveEnabled = true;

    @Column(name = "codex_sync_enabled", nullable = false)
    private boolean codexSyncEnabled = true;

    /** e.g. {@code uz-Latn}, {@code uz-Cyrl}, {@code tr}, {@code en}, {@code ar}. */
    @Column(name = "locale", nullable = false, length = 16)
    private String locale = "uz-Latn";

    @Column(name = "integrity_score", nullable = false)
    private int integrityScore = 0;

    @Enumerated(EnumType.STRING)
    @Column(name = "status", nullable = false, length = 16)
    private PlayerStatus status = PlayerStatus.ACTIVE;

    @Column(name = "erasure_requested_at")
    private Instant erasureRequestedAt;

    @Column(name = "last_seen_at")
    private Instant lastSeenAt;

    /**
     * Optimistic lock. Two devices can PATCH the profile concurrently (a console
     * changing locale while a PC changes telemetry consent); losing one silently
     * would make a setting appear to revert by itself.
     */
    @Version
    @Column(name = "version_lock")
    private Long versionLock;

    protected Player() {
        // JPA
    }

    public Player(String locale) {
        this.locale = Objects.requireNonNullElse(locale, "uz-Latn");
    }

    // ── Behaviour ───────────────────────────────────────────────────────────

    /**
     * Bu o'yinchining tanlovlari global statistikaga qo'shiladimi?
     *
     * @return {@code true} agar akkaunt faol va integrity ball chegaradan past
     */
    public boolean countsTowardAggregateStats() {
        return status == PlayerStatus.ACTIVE && integrityScore < STATS_EXCLUSION_THRESHOLD;
    }

    /**
     * Anti-cheat signalini qayd etadi.
     *
     * <p>Ball faqat o'sadi va hech qachon avtomatik jazo bermaydi — u yagona
     * narsani hal qiladi: {@link #countsTowardAggregateStats()}.
     *
     * @param weight severity of the signal; small for a bad client HMAC, larger
     *               for physically impossible telemetry
     */
    public void raiseIntegritySuspicion(int weight) {
        if (weight <= 0) {
            return;
        }
        this.integrityScore = Math.min(this.integrityScore + weight, 1_000);
    }

    /** Marks the account for GDPR erasure; the scrub itself runs nightly. */
    public void requestErasure() {
        this.status = PlayerStatus.ERASURE_PENDING;
        this.erasureRequestedAt = Instant.now();
    }

    public void touchLastSeen() {
        this.lastSeenAt = Instant.now();
    }

    public boolean isActive() {
        return status == PlayerStatus.ACTIVE;
    }

    /** Telemetry is produced only when the player has not opted out entirely. */
    public boolean allowsTelemetry() {
        return telemetryConsent != TelemetryConsent.OFF;
    }

    // ── Accessors ───────────────────────────────────────────────────────────

    public UUID getId() {
        return id;
    }

    public String getDisplayName() {
        return displayName;
    }

    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }

    public TelemetryConsent getTelemetryConsent() {
        return telemetryConsent;
    }

    public void setTelemetryConsent(TelemetryConsent telemetryConsent) {
        this.telemetryConsent = telemetryConsent;
    }

    public boolean isCloudSaveEnabled() {
        return cloudSaveEnabled;
    }

    public void setCloudSaveEnabled(boolean cloudSaveEnabled) {
        this.cloudSaveEnabled = cloudSaveEnabled;
    }

    public boolean isCodexSyncEnabled() {
        return codexSyncEnabled;
    }

    public void setCodexSyncEnabled(boolean codexSyncEnabled) {
        this.codexSyncEnabled = codexSyncEnabled;
    }

    public String getLocale() {
        return locale;
    }

    public void setLocale(String locale) {
        this.locale = locale;
    }

    public int getIntegrityScore() {
        return integrityScore;
    }

    public PlayerStatus getStatus() {
        return status;
    }

    public void setStatus(PlayerStatus status) {
        this.status = status;
    }

    public Instant getErasureRequestedAt() {
        return erasureRequestedAt;
    }

    public Instant getLastSeenAt() {
        return lastSeenAt;
    }

    // ── Identity semantics ──────────────────────────────────────────────────

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof Player other)) {
            return false;
        }
        // Unsaved entities are never equal to anything but themselves.
        return id != null && id.equals(other.id);
    }

    @Override
    public int hashCode() {
        return Player.class.hashCode();
    }

    @Override
    public String toString() {
        return "Player{id=%s, status=%s, locale=%s}".formatted(id, status, locale);
    }
}
