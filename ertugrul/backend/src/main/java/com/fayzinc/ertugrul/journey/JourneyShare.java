package com.fayzinc.ertugrul.journey;

import com.fayzinc.ertugrul.identity.Player;
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
 * A public link to a player's diary.
 *
 * <p><b>Bu marketing xususiyati.</b> Havolani ochgan odam o'yinni sotib
 * olmagan bo'lishi mumkin — aynan shu nuqta: o'yinchi o'z daftarini
 * ulashadi, uni ko'rgan odam o'yin haqida biladi. Shuning uchun havola
 * autentifikatsiyasiz ishlaydi.
 *
 * <p><b>Xavfsizlik.</b> Ommaviy havolada yagona himoya — <i>topib
 * bo'lmaslik</i>. Shuning uchun token kamida 128 bit entropiyaga ega. Undan
 * tashqari havola: muddatli ({@link #expiresAt}), bekor qilinadigan
 * ({@link #revokedAt}), va <b>faqat daftar matnini</b> ochadi — o'yinchi
 * ID'si, save ma'lumoti, pochta manzili hech qachon chiqmaydi.
 */
@Entity
@Table(name = "journey_share")
public class JourneyShare {

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "player_id", nullable = false, updatable = false)
    private Player player;

    @Column(name = "playthrough_id", nullable = false, updatable = false)
    private UUID playthroughId;

    @Column(name = "share_token", nullable = false, length = 64, updatable = false)
    private String shareToken;

    /** Shown on the public page instead of anything identifying. */
    @Column(name = "public_title", length = 96)
    private String publicTitle;

    @Column(name = "from_sequence_no")
    private Integer fromSequenceNo;

    /** Null means "everything so far", which keeps growing as the player plays. */
    @Column(name = "to_sequence_no")
    private Integer toSequenceNo;

    @Column(name = "view_count", nullable = false)
    private long viewCount;

    @Column(name = "expires_at", nullable = false)
    private Instant expiresAt;

    @Column(name = "revoked_at")
    private Instant revokedAt;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    protected JourneyShare() {
        // JPA
    }

    public JourneyShare(Player player, UUID playthroughId, String shareToken, Instant expiresAt) {
        this.player = player;
        this.playthroughId = playthroughId;
        this.shareToken = shareToken;
        this.expiresAt = expiresAt;
    }

    /** Whether the link still resolves. */
    public boolean isLive(Instant moment) {
        return revokedAt == null && moment.isBefore(expiresAt);
    }

    /**
     * Ko'rishlar sonini oshiradi.
     *
     * <p>Aniqlik muhim emas — bu marketing metrikasi, hisob-kitob emas.
     * Shuning uchun hech qanday qulf yo'q: parallel ko'rishlarda bir-ikkitasi
     * yo'qolsa ham zarar yo'q.
     */
    public void recordView() {
        this.viewCount++;
    }

    public void revoke() {
        if (this.revokedAt == null) {
            this.revokedAt = Instant.now();
        }
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

    public String getShareToken() {
        return shareToken;
    }

    public String getPublicTitle() {
        return publicTitle;
    }

    public void setPublicTitle(String publicTitle) {
        this.publicTitle = publicTitle;
    }

    public Integer getFromSequenceNo() {
        return fromSequenceNo;
    }

    public void setFromSequenceNo(Integer fromSequenceNo) {
        this.fromSequenceNo = fromSequenceNo;
    }

    public Integer getToSequenceNo() {
        return toSequenceNo;
    }

    public void setToSequenceNo(Integer toSequenceNo) {
        this.toSequenceNo = toSequenceNo;
    }

    public long getViewCount() {
        return viewCount;
    }

    public Instant getExpiresAt() {
        return expiresAt;
    }

    public Instant getRevokedAt() {
        return revokedAt;
    }

    public Instant getCreatedAt() {
        return createdAt;
    }
}
