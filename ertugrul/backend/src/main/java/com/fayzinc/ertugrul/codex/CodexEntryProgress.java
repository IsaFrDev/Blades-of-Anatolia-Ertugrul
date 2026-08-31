package com.fayzinc.ertugrul.codex;

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
 * One codex entry a player has unlocked.
 *
 * <p><b>Tarix qatlami — o'yinning asosiy maqsadi.</b> Kodeks Assassin's
 * Creed'ning "Database"idan bir pog'ona yuqori: u nafaqat ma'lumot beradi,
 * balki <b>ishonchlilik darajasini</b> ham ko'rsatadi. Shuning uchun
 * {@link Confidence} shu yerda saqlanadi — analitika "o'yinchilar bahsli
 * yozuvlarni o'qiydimi yoki faqat hujjatlilarni?" degan savolga javob
 * berishi kerak, va bu savol butun tarix qatlamining qiymatini o'lchaydi.
 *
 * <p><b>Sinxron modeli — o'suvchi to'plam (grow-only set).</b> Yozuv hech
 * qachon qayta qulflanmaydi, shuning uchun qurilmalar orasida birlashtirish
 * arzimas darajada sodda: <i>birlashma</i> (union). Konflikt bo'lishi mumkin
 * emas. Faqat {@link #readCount} va {@link #bookmarked} o'zgaruvchan, ular
 * {@link #revision} bo'yicha last-write-wins bilan hal qilinadi.
 */
@Entity
@Table(name = "codex_progress")
public class CodexEntryProgress {

    /**
     * How well-attested the entry is — the whole point of the history layer.
     *
     * <p>O'yin hech kimni ayblamaydi va hech narsani rad etmaydi. U faqat
     * "mana manbalar, mana olimlar fikri, o'zingiz xulosa qiling" deydi.
     */
    public enum Confidence {
        /** Confirmed by a contemporary or reliable source. Gold in the UI. */
        DOCUMENTED,
        /** Sources conflict; scholars disagree. Silver in the UI. */
        DISPUTED,
        /** Later-century tradition or artistic invention. Blue in the UI. */
        LEGEND
    }

    /** How the entry was discovered. Never automatic — always diegetic. */
    public enum UnlockMethod {
        /** ~40%: marked with BilgeGoz, e.g. looking at a tent's smoke hole. */
        OBSERVE,
        /** ~20%: using something, e.g. waxing a bowstring in the rain. */
        USE,
        /** ~20%: asking the right question of the right person. */
        DIALOGUE,
        /** ~12%: finding an object, e.g. a coin in ruins. */
        FIND,
        /** ~8%: living through an event, e.g. the battle of Kose Dag. */
        EVENT
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false, updatable = false)
    private Player player;

    /** e.g. {@code CDX_KAYI_TRIBE}. Content lives in the CDN, not here. */
    @Column(name = "codex_id", nullable = false, length = 64, updatable = false)
    private String codexId;

    @Enumerated(EnumType.STRING)
    @Column(name = "confidence", nullable = false, length = 12)
    private Confidence confidence;

    @Enumerated(EnumType.STRING)
    @Column(name = "category", nullable = false, length = 16)
    private CodexCategory category;

    @Enumerated(EnumType.STRING)
    @Column(name = "unlock_method", nullable = false, length = 12)
    private UnlockMethod unlockMethod;

    @Column(name = "episode_id", length = 5)
    private String episodeId;

    @Column(name = "unlocked_at", nullable = false)
    private Instant unlockedAt;

    /**
     * How many times the player actually opened it.
     *
     * <p>0 = ochilgan, lekin hech qachon o'qilmagan. <b>Ochilgan va
     * o'qilmagan orasidagi farq — tarix qatlamining samaradorligini o'lchaydigan
     * eng muhim raqam.</b> Agar o'yinchilar yozuvlarni ochib, lekin o'qimasa,
     * demak kodeks interfeysi yoki matn uzunligi ishlamayapti.
     */
    @Column(name = "read_count", nullable = false)
    private int readCount;

    @Column(name = "bookmarked", nullable = false)
    private boolean bookmarked;

    /** LWW counter for the mutable fields. */
    @Column(name = "revision", nullable = false)
    private long revision = 1;

    @Column(name = "origin_device_id", length = 64)
    private String originDeviceId;

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt = Instant.now();

    protected CodexEntryProgress() {
        // JPA
    }

    public CodexEntryProgress(Player player, String codexId, Confidence confidence,
                              CodexCategory category, UnlockMethod unlockMethod,
                              String episodeId, Instant unlockedAt, String originDeviceId) {
        this.player = player;
        this.codexId = codexId;
        this.confidence = confidence;
        this.category = category;
        this.unlockMethod = unlockMethod;
        this.episodeId = episodeId;
        this.unlockedAt = unlockedAt;
        this.originDeviceId = originDeviceId;
    }

    /**
     * O'zgaruvchan maydonlarni boshqa qurilmadan kelgan holat bilan
     * birlashtiradi.
     *
     * <p>Qoidalar:
     * <ul>
     *   <li>{@code readCount} — <b>maksimum</b> olinadi, yig'indi emas. Ikki
     *       qurilmadagi o'qishlar bir-biriga qo'shilsa, takroriy sinxron
     *       sonni sun'iy shishirib yuboradi;</li>
     *   <li>{@code bookmarked} — <b>revision</b> bo'yicha eng yangisi yutadi,
     *       chunki bu haqiqiy holat almashinuvi (qo'ydi/oldi);</li>
     *   <li>{@code unlockedAt} — <b>eng erta</b> vaqt saqlanadi: yozuv
     *       birinchi marta qachon ochilgani o'zgarmas fakt.</li>
     * </ul>
     *
     * @param incomingReadCount  read count reported by the syncing device
     * @param incomingBookmarked bookmark state reported by the syncing device
     * @param incomingRevision   the incoming revision counter
     * @param incomingUnlockedAt when that device thinks it was unlocked
     * @param deviceId           the syncing device
     * @return {@code true} when anything actually changed
     */
    public boolean mergeFrom(int incomingReadCount, boolean incomingBookmarked,
                             long incomingRevision, Instant incomingUnlockedAt,
                             String deviceId) {
        boolean changed = false;

        if (incomingReadCount > this.readCount) {
            this.readCount = incomingReadCount;
            changed = true;
        }

        if (incomingRevision > this.revision) {
            if (this.bookmarked != incomingBookmarked) {
                this.bookmarked = incomingBookmarked;
                changed = true;
            }
            this.revision = incomingRevision;
            changed = true;
        }

        if (incomingUnlockedAt != null && incomingUnlockedAt.isBefore(this.unlockedAt)) {
            this.unlockedAt = incomingUnlockedAt;
            changed = true;
        }

        if (changed) {
            this.originDeviceId = deviceId;
            this.updatedAt = Instant.now();
        }
        return changed;
    }

    /** Records that the player opened the entry. */
    public void markRead() {
        this.readCount++;
        this.revision++;
        this.updatedAt = Instant.now();
    }

    public void setBookmarked(boolean bookmarked) {
        if (this.bookmarked != bookmarked) {
            this.bookmarked = bookmarked;
            this.revision++;
            this.updatedAt = Instant.now();
        }
    }

    /** True when unlocked but never opened — the history layer's key failure mode. */
    public boolean isUnread() {
        return readCount == 0;
    }

    // ── Accessors ───────────────────────────────────────────────────────────

    public UUID getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public String getCodexId() {
        return codexId;
    }

    public Confidence getConfidence() {
        return confidence;
    }

    public CodexCategory getCategory() {
        return category;
    }

    public UnlockMethod getUnlockMethod() {
        return unlockMethod;
    }

    public String getEpisodeId() {
        return episodeId;
    }

    public Instant getUnlockedAt() {
        return unlockedAt;
    }

    public int getReadCount() {
        return readCount;
    }

    public boolean isBookmarked() {
        return bookmarked;
    }

    public long getRevision() {
        return revision;
    }

    public String getOriginDeviceId() {
        return originDeviceId;
    }

    public Instant getUpdatedAt() {
        return updatedAt;
    }
}
