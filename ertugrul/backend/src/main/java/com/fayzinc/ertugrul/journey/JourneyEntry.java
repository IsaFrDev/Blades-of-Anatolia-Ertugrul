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
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.time.Instant;
import java.time.LocalDate;
import java.util.List;
import java.util.UUID;

/**
 * One page of the "Safar Daftari" — the player's auto-written diary.
 *
 * <p><b>Nima uchun bu shunchaki quest log emas.</b> Daftar o'yin voqealarini
 * <i>ro'yxat</i> qilib emas, <b>o'yinchining o'z ovozida</b> yozadi. U
 * o'yinchining haqiqiy tanlovlaridan, uchrashgan odamlaridan va ochgan
 * kodekslaridan shakllanadi. Natijada tarix quruq faktga emas, <i>kechinmaga</i>
 * aylanadi — 02_HISTORY_LAYER.md §4 dagi asosiy g'oya shu.
 *
 * <p><b>Nima uchun serverda.</b> Uch sabab: qurilmalar orasida sinxron, PDF
 * eksport, va ommaviy havola — havolani ochgan odam o'yinni sotib olmagan
 * bo'lishi mumkin. Uchinchisi marketing xususiyati, va u save blob ichida
 * saqlab bo'lmaydi.
 *
 * <p><b>Matn loc-key emas, tayyor matn.</b> Yozuvlar ish vaqtida o'yinchi
 * tilida shablondan tug'iladi. Til o'zgarganda eski yozuvlarni qayta
 * tarjima qilish — o'yinchining <i>shaxsiy yozuvini</i> qayta yozish degani
 * bo'lardi, va bu noto'g'ri.
 */
@Entity
@Table(name = "journey_entry")
public class JourneyEntry {

    /**
     * The page's emotional colour, derived from world state at write time.
     *
     * <p>PDF'da siyoh rangi va hoshiya naqshini belgilaydi.
     */
    public enum Tone {
        NEUTRAL,
        HOPEFUL,
        GRIEVING,
        RESOLUTE,
        /** Written while the hand was in a bad way; the script is visibly unsteady. */
        WOUNDED
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false, updatable = false)
    private Player player;

    /**
     * Which playthrough this belongs to.
     *
     * <p>NG+ yangi daftar boshlaydi, eskisiga qo'shmaydi: ikkinchi o'tishda
     * o'yinchi boshqa tarixiy talqinlarni tanlaydi va ikki daftarni
     * aralashtirish ikkalasini ham ma'nosiz qilardi.
     */
    @Column(name = "playthrough_id", nullable = false, updatable = false)
    private UUID playthroughId;

    /** Client-assigned, gap-tolerant, so an offline device can keep writing. */
    @Column(name = "sequence_no", nullable = false, updatable = false)
    private int sequenceNo;

    @Column(name = "episode_id", nullable = false, length = 5)
    private String episodeId;

    @Column(name = "season_id", nullable = false, length = 2)
    private String seasonId;

    /**
     * Hijri date as written, e.g. {@code 632 Rabi al-awwal}.
     *
     * <p>Ikki taqvim butun o'yin bo'ylab yonma-yon ko'rsatiladi
     * (02_HISTORY_LAYER.md §5) — bu davr uchun tarixiy jihatdan to'g'ri va
     * o'yinchiga yangi bilim beradi.
     */
    @Column(name = "hijri_date_text", nullable = false, length = 64)
    private String hijriDateText;

    @Column(name = "gregorian_date_text", nullable = false, length = 64)
    private String gregorianDateText;

    /** Sortable anchor for timeline rendering and PDF ordering. */
    @Column(name = "in_game_date", nullable = false)
    private LocalDate inGameDate;

    @Column(name = "place_name", length = 96)
    private String placeName;

    @Column(name = "body", nullable = false, columnDefinition = "text")
    private String body;

    /** Codex references shown at the foot of the page. */
    @JdbcTypeCode(SqlTypes.ARRAY)
    @Column(name = "linked_codex_ids", columnDefinition = "text[]")
    private List<String> linkedCodexIds = List.of();

    @Enumerated(EnumType.STRING)
    @Column(name = "tone", nullable = false, length = 16)
    private Tone tone = Tone.NEUTRAL;

    /**
     * True for entries written after EP024.
     *
     * <p>PDF'da ular boshqa, beqaror qo'lyozma bilan chiziladi. Kichik detal,
     * lekin butun jarohat tizimining ma'nosi shunda: o'yinchi o'z daftarini
     * qayta o'qiganda, qaysi sahifadan keyin qo'l o'zgarganini <i>ko'radi</i>.
     */
    @Column(name = "written_left_handed", nullable = false)
    private boolean writtenLeftHanded;

    /** Player can hide a page from a shared diary without deleting it. */
    @Column(name = "hidden_from_share", nullable = false)
    private boolean hiddenFromShare;

    @Column(name = "origin_device_id", length = 64)
    private String originDeviceId;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt = Instant.now();

    protected JourneyEntry() {
        // JPA
    }

    public JourneyEntry(Player player, UUID playthroughId, int sequenceNo,
                        String episodeId, String seasonId,
                        String hijriDateText, String gregorianDateText, LocalDate inGameDate,
                        String body) {
        this.player = player;
        this.playthroughId = playthroughId;
        this.sequenceNo = sequenceNo;
        this.episodeId = episodeId;
        this.seasonId = seasonId;
        this.hijriDateText = hijriDateText;
        this.gregorianDateText = gregorianDateText;
        this.inGameDate = inGameDate;
        this.body = body;
    }

    /** Combined dual-calendar heading, e.g. "632 Rabi al-awwal / 1234 December". */
    public String dualDateHeading() {
        return hijriDateText + "  /  " + gregorianDateText;
    }

    public void setHiddenFromShare(boolean hidden) {
        this.hiddenFromShare = hidden;
        this.updatedAt = Instant.now();
    }

    // ── Accessors ───────────────────────────────────────────────────────────

    public UUID getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public UUID getPlaythroughId() {
        return playthroughId;
    }

    public int getSequenceNo() {
        return sequenceNo;
    }

    public String getEpisodeId() {
        return episodeId;
    }

    public String getSeasonId() {
        return seasonId;
    }

    public String getHijriDateText() {
        return hijriDateText;
    }

    public String getGregorianDateText() {
        return gregorianDateText;
    }

    public LocalDate getInGameDate() {
        return inGameDate;
    }

    public String getPlaceName() {
        return placeName;
    }

    public void setPlaceName(String placeName) {
        this.placeName = placeName;
    }

    public String getBody() {
        return body;
    }

    public List<String> getLinkedCodexIds() {
        return linkedCodexIds;
    }

    public void setLinkedCodexIds(List<String> linkedCodexIds) {
        this.linkedCodexIds = linkedCodexIds == null ? List.of() : List.copyOf(linkedCodexIds);
    }

    public Tone getTone() {
        return tone;
    }

    public void setTone(Tone tone) {
        this.tone = tone == null ? Tone.NEUTRAL : tone;
    }

    public boolean isWrittenLeftHanded() {
        return writtenLeftHanded;
    }

    public void setWrittenLeftHanded(boolean writtenLeftHanded) {
        this.writtenLeftHanded = writtenLeftHanded;
    }

    public boolean isHiddenFromShare() {
        return hiddenFromShare;
    }

    public String getOriginDeviceId() {
        return originDeviceId;
    }

    public void setOriginDeviceId(String originDeviceId) {
        this.originDeviceId = originDeviceId;
    }

    public Instant getCreatedAt() {
        return createdAt;
    }

    public Instant getUpdatedAt() {
        return updatedAt;
    }
}
