package com.fayzinc.ertugrul.save;

import com.fayzinc.ertugrul.common.Auditable;
import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.save.dto.WorldStateSummary;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.Table;
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.util.Map;
import java.util.UUID;

/**
 * One cloud-save slot: 8 manual slots plus slot 0 for the engine's autosave.
 *
 * <p>Bu qator <b>blob'ning o'zini saqlamaydi</b>. Blob S3/MinIO'da, bu yerda
 * esa metadata, vector clock va yuklash ekrani uchun kichik "xulosa" turadi.
 * Sabab sodda: "Davom etish" menyusi 9 ta slotni ko'rsatadi va u S3'ga 9 marta
 * murojaat qilmasligi kerak — bu menyuni sekundlarga cho'zadi.
 *
 * <p>Xulosadagi {@code handIntegrity} maxsus e'tibor talab qiladi: o'yin HUD'da
 * <b>raqam ko'rsatmaydi</b> (05_MIH_SYSTEM.md §7), lekin klient qaysi qo'l
 * ikonkasini chizishni bilishi uchun qiymat kerak.
 */
@Entity
@Table(name = "save_slot")
public class SaveSlot extends Auditable {

    /** Slot reserved for the engine-driven autosave. */
    public static final int AUTOSAVE_SLOT = 0;

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false, updatable = false)
    private Player player;

    @Column(name = "slot_index", nullable = false, updatable = false)
    private short slotIndex;

    /** Monotonic per slot; points at the winning {@link SaveVersion}. */
    @Column(name = "head_version", nullable = false)
    private long headVersion;

    /**
     * Merged vector clock of the head.
     *
     * <p>Hibernate 6 maps this straight onto {@code jsonb}. Denormalised here so
     * the pre-flight conflict check is one row read rather than a join.
     */
    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "vector_clock", nullable = false, columnDefinition = "jsonb")
    private Map<String, Long> vectorClock = Map.of();

    // ── Load-screen summary ─────────────────────────────────────────────────

    @Column(name = "episode_id", length = 5)
    private String episodeId;

    @Column(name = "season_id", length = 2)
    private String seasonId;

    @Column(name = "playtime_seconds", nullable = false)
    private long playtimeSeconds;

    @Column(name = "hand_integrity", nullable = false)
    private float handIntegrity = 100f;

    /** The ceiling. 100 until EP024, then 55; only the EP043 prosthesis raises it. */
    @Column(name = "max_integrity", nullable = false)
    private float maxIntegrity = 100f;

    @Column(name = "difficulty_tier", length = 16)
    private String difficultyTier;

    /** Blob layout version; the client refuses to load a blob newer than it knows. */
    @Column(name = "schema_version", nullable = false)
    private int schemaVersion = 1;

    /** True while an unresolved conflict copy exists for this slot. */
    @Column(name = "has_conflict", nullable = false)
    private boolean hasConflict;

    protected SaveSlot() {
        // JPA
    }

    public SaveSlot(Player player, int slotIndex) {
        this.player = player;
        this.slotIndex = (short) slotIndex;
    }

    // ── Behaviour ───────────────────────────────────────────────────────────

    public VectorClock clock() {
        return VectorClock.of(vectorClock);
    }

    /**
     * Slot boshini yangi versiyaga o'tkazadi.
     *
     * <p>Vector clock birlashtiriladi va {@link VectorClock#MAX_NODES} gacha
     * siqiladi: o'yinchi qurilma almashtiraversa soat cheksiz o'sib, har save
     * bilan tashilib yuradi.
     *
     * @param version  the new head version number
     * @param newClock the incoming clock, already merged with the previous head
     * @param summary  denormalised state for the load screen
     */
    public void advanceHead(long version, VectorClock newClock, WorldStateSummary summary) {
        this.headVersion = version;
        this.vectorClock = newClock.compact().counters();

        if (summary != null) {
            this.episodeId = summary.episodeId();
            this.seasonId = summary.seasonId();
            this.playtimeSeconds = summary.playtimeSeconds();
            this.handIntegrity = summary.handIntegrity();
            this.maxIntegrity = summary.maxIntegrity();
            this.difficultyTier = summary.difficultyTier() == null
                    ? null : summary.difficultyTier().name();
            this.schemaVersion = summary.schemaVersion();
        }
    }

    public void markConflict(boolean conflict) {
        this.hasConflict = conflict;
    }

    public boolean isAutosave() {
        return slotIndex == AUTOSAVE_SLOT;
    }

    /**
     * Ushbu slot hech qachon yozilganmi?
     *
     * @return {@code true} if no save has ever been uploaded here
     */
    public boolean isEmpty() {
        return headVersion == 0;
    }

    // ── Accessors ───────────────────────────────────────────────────────────

    public UUID getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public int getSlotIndex() {
        return slotIndex;
    }

    public long getHeadVersion() {
        return headVersion;
    }

    public Map<String, Long> getVectorClock() {
        return vectorClock;
    }

    public String getEpisodeId() {
        return episodeId;
    }

    public String getSeasonId() {
        return seasonId;
    }

    public long getPlaytimeSeconds() {
        return playtimeSeconds;
    }

    public float getHandIntegrity() {
        return handIntegrity;
    }

    public float getMaxIntegrity() {
        return maxIntegrity;
    }

    public String getDifficultyTier() {
        return difficultyTier;
    }

    public int getSchemaVersion() {
        return schemaVersion;
    }

    public boolean isHasConflict() {
        return hasConflict;
    }

    @Override
    public String toString() {
        return "SaveSlot{slot=%d, head=%d, episode=%s}".formatted(slotIndex, headVersion, episodeId);
    }
}
