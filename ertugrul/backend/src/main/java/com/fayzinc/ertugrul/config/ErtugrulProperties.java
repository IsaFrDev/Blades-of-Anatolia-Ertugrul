package com.fayzinc.ertugrul.config;

import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.boot.context.properties.bind.DefaultValue;
import org.springframework.validation.annotation.Validated;

import java.time.Duration;

/**
 * Typed binding for every {@code ertugrul.*} key in {@code application.yml}.
 *
 * <p>Bitta joyda to'plangani ataylab: balans va limit qiymatlari (save slot
 * soni, blob hajmi, rate limit) — bu <b>o'yin dizayni qarorlari</b>, texnik
 * detal emas. Ular kod ichida tarqalib ketsa, dizayner ularni topolmaydi va
 * o'zgartira olmaydi. Shuning uchun hammasi bitta konfiguratsiya sinfida.
 *
 * @param security JWT and HMAC secrets
 * @param s3       object store endpoints and buckets
 * @param save     cloud-save limits driven by game design
 * @param telemetry Kafka topics and ingest bounds
 * @param liveops  remote config cache and cohort bucketing
 * @param ratelimit per-route-class token bucket sizes
 * @param journey  Safar Daftari quotas and link lifetimes
 */
@Validated
@ConfigurationProperties(prefix = "ertugrul")
public record ErtugrulProperties(

        Security security,
        S3 s3,
        Save save,
        Telemetry telemetry,
        LiveOps liveops,
        RateLimit ratelimit,
        Journey journey
) {

    /**
     * @param jwt               token issuance/validation settings
     * @param saveHmacSecret    server-side key for save blob signatures
     * @param clientHmacSecret  key shared with the shipped client; SOFT signal only
     */
    public record Security(
            Jwt jwt,
            @NotBlank String saveHmacSecret,
            @NotBlank String clientHmacSecret
    ) {
        /**
         * @param issuer          {@code iss} claim
         * @param audience        {@code aud} claim
         * @param accessTokenTtl  short — the client refreshes silently in menus
         * @param refreshTokenTtl long — a player may not launch the game for weeks
         * @param privateKeyPem   RSA private key; empty means "generate ephemeral" (dev only)
         * @param publicKeyPem    RSA public key
         */
        public record Jwt(
                @NotBlank String issuer,
                @NotBlank String audience,
                @DefaultValue("PT30M") Duration accessTokenTtl,
                @DefaultValue("P30D") Duration refreshTokenTtl,
                @DefaultValue("") String privateKeyPem,
                @DefaultValue("") String publicKeyPem
        ) {
        }
    }

    /**
     * @param endpoint        MinIO or S3 endpoint
     * @param pathStyleAccess MinIO requires true; real S3 does not
     * @param saveBucket      versioned bucket for save blobs
     * @param exportBucket    public-read bucket for Safar Daftari PDFs
     * @param presignTtl      lifetime of presigned upload/download URLs
     */
    public record S3(
            @NotBlank String endpoint,
            @DefaultValue("us-east-1") String region,
            @NotBlank String accessKey,
            @NotBlank String secretKey,
            @DefaultValue("true") boolean pathStyleAccess,
            @DefaultValue("ertugrul-saves") String saveBucket,
            @DefaultValue("ertugrul-exports") String exportBucket,
            @DefaultValue("PT15M") Duration presignTtl
    ) {
    }

    /**
     * @param maxManualSlots          8 manual slots, per the settings design
     * @param autosaveSlotIndex       slot 0 is engine-owned autosave
     * @param maxBlobBytes            hard ceiling on a single save blob
     * @param retainedVersionsPerSlot version tail kept for support restores
     * @param maxClockSkew            tolerance on the untrusted client timestamp
     */
    public record Save(
            @DefaultValue("8") @Min(1) @Max(32) int maxManualSlots,
            @DefaultValue("0") int autosaveSlotIndex,
            @DefaultValue("8388608") @Min(1024) long maxBlobBytes,
            @DefaultValue("10") @Min(1) int retainedVersionsPerSlot,
            @DefaultValue("PT10M") Duration maxClockSkew
    ) {
        /** Total addressable slots: autosave + manual. */
        public int totalSlots() {
            return maxManualSlots + 1;
        }

        /** Whether {@code index} is a slot this deployment accepts. */
        public boolean isValidSlot(int index) {
            return index >= 0 && index <= maxManualSlots;
        }
    }

    /**
     * @param topic        raw event topic
     * @param dlqTopic     events rejected by the sanity checker
     * @param maxBatchSize per-request event cap
     * @param maxEventAge  older events are dropped (stale offline spool)
     */
    public record Telemetry(
            @DefaultValue("ert.telemetry.raw") @NotBlank String topic,
            @DefaultValue("ert.telemetry.rejected") @NotBlank String dlqTopic,
            @DefaultValue("200") @Min(1) @Max(1000) int maxBatchSize,
            @DefaultValue("P3D") Duration maxEventAge
    ) {
    }

    /**
     * @param remoteConfigCacheTtl Redis TTL for the resolved config document
     * @param cohortSalt           A/B bucketing salt — never change mid-experiment
     * @param codexCdnBaseUrl      where new codex packs are published
     */
    public record LiveOps(
            @DefaultValue("PT60S") Duration remoteConfigCacheTtl,
            @NotBlank String cohortSalt,
            @DefaultValue("") String codexCdnBaseUrl
    ) {
    }

    /**
     * Per-minute token buckets. Auth is tightest (credential stuffing), telemetry
     * loosest (a legitimate client batches every few seconds).
     */
    public record RateLimit(
            @DefaultValue("true") boolean enabled,
            @DefaultValue("10") int authPerMinute,
            @DefaultValue("20") int saveUploadPerMinute,
            @DefaultValue("120") int telemetryPerMinute,
            @DefaultValue("300") int defaultPerMinute
    ) {
    }

    /**
     * @param maxEntriesPerPlayer guards against a runaway client writing forever
     * @param shareLinkTtl        public links expire; they are not permanent hosting
     * @param exportTtl           generated PDFs are cleaned up after this
     */
    public record Journey(
            @DefaultValue("2000") @Min(1) int maxEntriesPerPlayer,
            @DefaultValue("P90D") Duration shareLinkTtl,
            @DefaultValue("P7D") Duration exportTtl
    ) {
    }
}
