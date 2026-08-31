package com.fayzinc.ertugrul.liveops;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.time.Instant;
import java.util.Map;
import java.util.UUID;

/**
 * A time-boxed live-ops window.
 *
 * <p>Masalan: Köse Dağ jangining yilligi (26-iyun), Discovery Tour dam olish
 * kunlari, yoki yangi kodeks paketining chiqishi.
 *
 * <p><b>Muhim cheklov:</b> mavsumiy hodisa <b>hikoyaga ta'sir qilmaydi</b>.
 * O'yin single-player va uni istalgan vaqtda, offline o'ynash mumkin. Agar
 * hodisa syujet kontentini qulflasa yoki ochsa, o'yin "xizmat" (live-service)
 * ga aylanib qolardi — bu esa loyihaning butun falsafasiga zid. Hodisalar
 * faqat <i>qo'shimcha</i> narsa beradi: kodeks paketi, ekskursiya, e'lon.
 */
@Entity
@Table(name = "seasonal_event")
public class SeasonalEvent {

    public enum EventType {
        /** New codex entries published to the CDN; no client patch required. */
        CODEX_DROP,
        /** A Discovery Tour ("Sayyoh rejimi") weekend. */
        DISCOVERY_TOUR,
        /** Historical anniversary, e.g. Kose Dag on 26 June. */
        ANNIVERSARY,
        /** A temporary balance experiment window. */
        BALANCE_WEEKEND,
        /** A plain message on the main menu. */
        ANNOUNCEMENT
    }

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @Column(name = "event_key", nullable = false, length = 64, updatable = false)
    private String eventKey;

    /** Localisation keys, not text: events are shown in the player's language. */
    @Column(name = "title_loc_key", nullable = false, length = 96)
    private String titleLocKey;

    @Column(name = "body_loc_key", length = 96)
    private String bodyLocKey;

    @Enumerated(EnumType.STRING)
    @Column(name = "event_type", nullable = false, length = 24)
    private EventType eventType;

    /** For {@link EventType#CODEX_DROP}: the CDN manifest of new entries. */
    @Column(name = "cdn_manifest_url", length = 256)
    private String cdnManifestUrl;

    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "payload", nullable = false, columnDefinition = "jsonb")
    private Map<String, Object> payload = Map.of();

    @Column(name = "starts_at", nullable = false)
    private Instant startsAt;

    @Column(name = "ends_at", nullable = false)
    private Instant endsAt;

    @Column(name = "enabled", nullable = false)
    private boolean enabled = true;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    protected SeasonalEvent() {
        // JPA
    }

    public SeasonalEvent(String eventKey, String titleLocKey, EventType eventType,
                         Instant startsAt, Instant endsAt) {
        this.eventKey = eventKey;
        this.titleLocKey = titleLocKey;
        this.eventType = eventType;
        this.startsAt = startsAt;
        this.endsAt = endsAt;
    }

    public boolean isLive(Instant moment) {
        return enabled && !moment.isBefore(startsAt) && moment.isBefore(endsAt);
    }

    public UUID getId() {
        return id;
    }

    public String getEventKey() {
        return eventKey;
    }

    public String getTitleLocKey() {
        return titleLocKey;
    }

    public String getBodyLocKey() {
        return bodyLocKey;
    }

    public void setBodyLocKey(String bodyLocKey) {
        this.bodyLocKey = bodyLocKey;
    }

    public EventType getEventType() {
        return eventType;
    }

    public String getCdnManifestUrl() {
        return cdnManifestUrl;
    }

    public void setCdnManifestUrl(String cdnManifestUrl) {
        this.cdnManifestUrl = cdnManifestUrl;
    }

    public Map<String, Object> getPayload() {
        return payload;
    }

    public void setPayload(Map<String, Object> payload) {
        this.payload = payload == null ? Map.of() : payload;
    }

    public Instant getStartsAt() {
        return startsAt;
    }

    public Instant getEndsAt() {
        return endsAt;
    }

    public boolean isEnabled() {
        return enabled;
    }

    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }
}
