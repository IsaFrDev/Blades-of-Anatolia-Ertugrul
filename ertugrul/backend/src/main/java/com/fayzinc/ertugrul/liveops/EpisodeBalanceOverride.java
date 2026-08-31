package com.fayzinc.ertugrul.liveops;

import com.fayzinc.ertugrul.save.DifficultyTier;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

import java.time.Instant;
import java.util.UUID;

/**
 * Per-episode difficulty tuning, delivered without a client patch.
 *
 * <p><b>Bu — live-ops'ning eng o'tkir quroli.</b> Telemetriya EP029 da
 * o'rtacha {@code hand_integrity < 15} va o'lim {@code > 6} ekanini
 * ko'rsatganda (05_MIH_SYSTEM.md §8), jamoa shu yerdan
 * {@code woundDecayScale} ni pasaytiradi va o'zgarish bir soatda barcha
 * o'yinchilarga yetadi.
 *
 * <p><b>Maydonlar klientdagi {@code UErtGameUserSettings} bilan 1:1 mos
 * keladi</b> (07_SETTINGS_HOTKEYS.md §5.3). Bu ataylab: override o'yinchi
 * o'zi tanlagan accessibility sozlamasi bilan <i>bir xil kod yo'lidan</i>
 * qo'llaniladi. Ya'ni u allaqachon sinovdan o'tgan va o'yin render qila
 * olmaydigan qiymatga hech qachon yetib bormaydi.
 *
 * <p><b>Chegaralar SQL'da ham qo'yilgan.</b> Panel orqali kiritilgan xato
 * qiymat ({@code enemyDamageScale = 25}) epizodni o'tib bo'lmas qilib
 * qo'yishi mumkin. Cheklovni faqat kodda emas, bazada ushlash — noto'g'ri
 * ma'lumot umuman yozilmasligini kafolatlaydi.
 */
@Entity
@Table(name = "episode_balance_override")
public class EpisodeBalanceOverride {

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @Column(name = "episode_id", nullable = false, length = 5, updatable = false)
    private String episodeId;

    /** Null means the override applies to every difficulty tier. */
    @Enumerated(EnumType.STRING)
    @Column(name = "difficulty_tier", length = 16)
    private DifficultyTier difficultyTier;

    /**
     * Parry window adjustment in milliseconds.
     *
     * <p>Parry — jang tizimining yuragi (04_CORE_SYSTEMS.md §1.3): sog'lom
     * qo'lda 180 ms, mixdan keyin 110–165 ms. Chegara {@code -60..+120}:
     * pastroq qiymat parry'ni imkonsiz, yuqorirog'i esa ma'nosiz qiladi.
     */
    @Column(name = "parry_window_ms_delta", nullable = false)
    private int parryWindowMsDelta;

    @Column(name = "enemy_damage_scale", nullable = false)
    private float enemyDamageScale = 1.0f;

    /**
     * Wound decay multiplier — the lever pulled when an episode is flagged as
     * too punishing.
     */
    @Column(name = "wound_decay_scale", nullable = false)
    private float woundDecayScale = 1.0f;

    @Column(name = "enemy_count_scale", nullable = false)
    private float enemyCountScale = 1.0f;

    /** 0..100. Two overrides with disjoint cohorts is how an A/B test is run. */
    @Column(name = "rollout_percent", nullable = false)
    private short rolloutPercent = 100;

    /** Distinguishes variant A from B when two experiments share an episode. */
    @Column(name = "variant_key", nullable = false, length = 32)
    private String variantKey = "default";

    @Column(name = "enabled", nullable = false)
    private boolean enabled = true;

    @Column(name = "notes")
    private String notes;

    @Column(name = "active_from", nullable = false)
    private Instant activeFrom = Instant.now();

    @Column(name = "active_until")
    private Instant activeUntil;

    @Column(name = "revision", nullable = false)
    private long revision = 1;

    @Column(name = "updated_by", length = 64)
    private String updatedBy;

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt = Instant.now();

    protected EpisodeBalanceOverride() {
        // JPA
    }

    public EpisodeBalanceOverride(String episodeId, String variantKey) {
        this.episodeId = episodeId;
        this.variantKey = variantKey == null || variantKey.isBlank() ? "default" : variantKey;
    }

    // ── Behaviour ───────────────────────────────────────────────────────────

    /**
     * Shu override o'yinchiga tegishlimi?
     *
     * @param cohortBucket the player's stable cohort bucket, 0..99
     * @param tier         the difficulty this playthrough runs at
     * @param moment       evaluation time
     * @return whether the client should receive this override
     */
    public boolean appliesTo(int cohortBucket, DifficultyTier tier, Instant moment) {
        if (!enabled) {
            return false;
        }
        if (moment.isBefore(activeFrom)) {
            return false;
        }
        if (activeUntil != null && !moment.isBefore(activeUntil)) {
            return false;
        }
        if (difficultyTier != null && difficultyTier != tier) {
            return false;
        }
        return cohortBucket < rolloutPercent;
    }

    /**
     * Balans qiymatlarini yangilaydi.
     *
     * <p>Chegaralar bu yerda ham tekshiriladi, garchi SQL'da ham bor bo'lsa —
     * bazadagi cheklov <i>oxirgi</i> himoya, bu yerdagi tekshiruv esa
     * panelga tushunarli xato qaytarish uchun.
     *
     * @throws IllegalArgumentException when a value is outside its designed range
     */
    public void retune(int parryDelta, float damageScale, float woundScale,
                       float countScale, String editor) {

        requireInRange("parryWindowMsDelta", parryDelta, -60, 120);
        requireInRange("enemyDamageScale", damageScale, 0.25f, 2.5f);
        requireInRange("woundDecayScale", woundScale, 0.2f, 2.0f);
        requireInRange("enemyCountScale", countScale, 0.5f, 2.0f);

        this.parryWindowMsDelta = parryDelta;
        this.enemyDamageScale = damageScale;
        this.woundDecayScale = woundScale;
        this.enemyCountScale = countScale;
        this.revision++;
        this.updatedBy = editor;
        this.updatedAt = Instant.now();
    }

    public void setRollout(short percent) {
        if (percent < 0 || percent > 100) {
            throw new IllegalArgumentException("rolloutPercent must be 0..100");
        }
        this.rolloutPercent = percent;
        this.revision++;
        this.updatedAt = Instant.now();
    }

    private static void requireInRange(String field, float value, float min, float max) {
        if (value < min || value > max) {
            throw new IllegalArgumentException(
                    "%s must be between %s and %s, got %s".formatted(field, min, max, value));
        }
    }

    // ── Accessors ───────────────────────────────────────────────────────────

    public UUID getId() {
        return id;
    }

    public String getEpisodeId() {
        return episodeId;
    }

    public DifficultyTier getDifficultyTier() {
        return difficultyTier;
    }

    public void setDifficultyTier(DifficultyTier difficultyTier) {
        this.difficultyTier = difficultyTier;
    }

    public int getParryWindowMsDelta() {
        return parryWindowMsDelta;
    }

    public float getEnemyDamageScale() {
        return enemyDamageScale;
    }

    public float getWoundDecayScale() {
        return woundDecayScale;
    }

    public float getEnemyCountScale() {
        return enemyCountScale;
    }

    public short getRolloutPercent() {
        return rolloutPercent;
    }

    public String getVariantKey() {
        return variantKey;
    }

    public boolean isEnabled() {
        return enabled;
    }

    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }

    public String getNotes() {
        return notes;
    }

    public void setNotes(String notes) {
        this.notes = notes;
    }

    public Instant getActiveFrom() {
        return activeFrom;
    }

    public Instant getActiveUntil() {
        return activeUntil;
    }

    public void setActiveUntil(Instant activeUntil) {
        this.activeUntil = activeUntil;
    }

    public long getRevision() {
        return revision;
    }

    public Instant getUpdatedAt() {
        return updatedAt;
    }
}
