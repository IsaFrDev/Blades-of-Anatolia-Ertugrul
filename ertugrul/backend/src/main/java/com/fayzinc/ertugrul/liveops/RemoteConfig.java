package com.fayzinc.ertugrul.liveops;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.time.Instant;
import java.util.Map;
import java.util.UUID;

/**
 * A remotely tunable value delivered without a client patch.
 *
 * <p><b>Nima uchun kerak.</b> Konsol patch'i sertifikatsiyadan o'tishi bir-ikki
 * hafta oladi. Agar EP029 chiqqandan keyin juda qiyin ekani ma'lum bo'lsa, ikki
 * hafta kutish — o'sha epizodda o'yinchilarni yo'qotish degani. Remote config
 * shu vaqtni <i>bir soatga</i> qisqartiradi.
 *
 * <p><b>Xavfsizlik qoidasi.</b> Yomon remote config <b>hech qachon o'yinni
 * buzmasligi kerak</b>. Klient har qiymatni o'z sxemasi bo'yicha tekshiradi va
 * tanimagan yoki chegaradan chiqqan qiymatni <i>jimgina</i> tashlab, o'zining
 * shipped default'iga qaytadi. Single-player o'yinda server xatosi o'yinchini
 * to'xtatib qo'ymasligi shart.
 */
@Entity
@Table(name = "remote_config")
public class RemoteConfig {

    @Id
    @GeneratedValue
    @Column(name = "id", nullable = false, updatable = false)
    private UUID id;

    @Column(name = "config_key", nullable = false, length = 96, updatable = false)
    private String configKey;

    /** Arbitrary JSON; the client owns its shape and validates it. */
    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "config_value", nullable = false, columnDefinition = "jsonb")
    private Map<String, Object> configValue = Map.of();

    /**
     * Rollout cohort, 0..100.
     *
     * <p>Bucketing — {@code (playerId + salt)} ning barqaror hash'i, shuning
     * uchun o'yinchi sessiyalar orasida kogortadan kogortaga sakramaydi. Bu
     * shart: A/B tajribasida o'yinchi bir kun oson, ertasiga qiyin balansni
     * ko'rsa, natija ham buziladi, tajriba ham.
     */
    @Column(name = "rollout_percent", nullable = false)
    private short rolloutPercent = 100;

    @Column(name = "min_app_version", length = 32)
    private String minAppVersion;

    @Column(name = "platform", length = 16)
    private String platform;

    @Column(name = "active_from", nullable = false)
    private Instant activeFrom = Instant.now();

    @Column(name = "active_until")
    private Instant activeUntil;

    /** Bumped on every edit; returned to the client as an ETag. */
    @Column(name = "revision", nullable = false)
    private long revision = 1;

    @Column(name = "updated_by", length = 64)
    private String updatedBy;

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt = Instant.now();

    protected RemoteConfig() {
        // JPA
    }

    public RemoteConfig(String configKey, Map<String, Object> configValue, int rolloutPercent) {
        this.configKey = configKey;
        this.configValue = configValue == null ? Map.of() : configValue;
        this.rolloutPercent = (short) rolloutPercent;
    }

    /** Whether this config is inside its activation window right now. */
    public boolean isActiveAt(Instant moment) {
        if (moment.isBefore(activeFrom)) {
            return false;
        }
        return activeUntil == null || moment.isBefore(activeUntil);
    }

    /**
     * Whether a player in the given cohort bucket receives this config.
     *
     * @param cohortBucket the player's stable bucket, 0..99
     */
    public boolean appliesToCohort(int cohortBucket) {
        return cohortBucket < rolloutPercent;
    }

    public void update(Map<String, Object> newValue, int newRollout, String editor) {
        this.configValue = newValue == null ? Map.of() : newValue;
        this.rolloutPercent = (short) newRollout;
        this.revision++;
        this.updatedBy = editor;
        this.updatedAt = Instant.now();
    }

    public UUID getId() {
        return id;
    }

    public String getConfigKey() {
        return configKey;
    }

    public Map<String, Object> getConfigValue() {
        return configValue;
    }

    public short getRolloutPercent() {
        return rolloutPercent;
    }

    public String getMinAppVersion() {
        return minAppVersion;
    }

    public void setMinAppVersion(String minAppVersion) {
        this.minAppVersion = minAppVersion;
    }

    public String getPlatform() {
        return platform;
    }

    public void setPlatform(String platform) {
        this.platform = platform;
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
