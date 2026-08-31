package com.fayzinc.ertugrul;

import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.identity.PlayerRepository;
import com.fayzinc.ertugrul.integrity.SaveSigner;
import com.fayzinc.ertugrul.save.DifficultyTier;
import com.fayzinc.ertugrul.save.HandPhase;
import com.fayzinc.ertugrul.save.SaveService;
import com.fayzinc.ertugrul.save.dto.SaveSlotSummary;
import com.fayzinc.ertugrul.save.dto.SaveUploadRequest;
import com.fayzinc.ertugrul.save.dto.SaveUploadResponse;
import com.fayzinc.ertugrul.save.dto.WorldStateSummary;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.testcontainers.service.connection.ServiceConnection;
import org.springframework.test.context.ActiveProfiles;
import org.testcontainers.containers.MinIOContainer;
import org.testcontainers.containers.PostgreSQLContainer;
import org.testcontainers.junit.jupiter.Container;
import org.testcontainers.junit.jupiter.Testcontainers;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Base64;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import static org.assertj.core.api.Assertions.assertThat;

/**
 * End-to-end cloud-save flow against real Postgres and MinIO.
 *
 * <p><b>Nima uchun Testcontainers, mock emas.</b> Save yo'lidagi eng nozik
 * qismlar — Flyway migratsiyalari, {@code jsonb} vector clock mapping'i,
 * pessimistic qulf va S3 obyekt kaliti — mock bilan <i>umuman</i>
 * tekshirilmaydi. Ular faqat haqiqiy Postgres va haqiqiy object store bilan
 * ishlaganda ma'noga ega.
 *
 * <p>Kafka va Redis bu testda ko'tarilmaydi: save yo'li ularga bog'liq emas
 * (rate limiter fail-open, telemetriya alohida yo'l), va ikkita ortiqcha
 * konteyner har bir yurishga sekundlar qo'shadi.
 */
@SpringBootTest
@Testcontainers
@ActiveProfiles("test")
class SaveFlowIntegrationTest {

    @Container
    @ServiceConnection
    static PostgreSQLContainer<?> postgres = new PostgreSQLContainer<>("postgres:16-alpine");

    @Container
    static MinIOContainer minio = new MinIOContainer("minio/minio:RELEASE.2024-09-22T00-33-43Z");

    @org.springframework.test.context.DynamicPropertySource
    static void wireMinio(org.springframework.test.context.DynamicPropertyRegistry registry) {
        registry.add("ertugrul.s3.endpoint", minio::getS3URL);
        registry.add("ertugrul.s3.access-key", minio::getUserName);
        registry.add("ertugrul.s3.secret-key", minio::getPassword);
    }

    @Autowired
    private SaveService saveService;

    @Autowired
    private PlayerRepository playerRepository;

    @Autowired
    private SaveSigner saveSigner;

    @Autowired
    private software.amazon.awssdk.services.s3.S3Client s3;

    private UUID playerId;

    /**
     * Buckets are provisioned outside the application: by {@code minio-init} in
     * docker-compose, and by infrastructure-as-code in production. The test has
     * to stand in for that, which is why it happens here and not in
     * {@code S3Config} — production code must not be creating its own buckets.
     */
    @BeforeEach
    void setUp() {
        ensureBucket("ertugrul-saves");
        ensureBucket("ertugrul-exports");
        playerId = playerRepository.save(new Player("uz-Latn")).getId();
    }

    private void ensureBucket(String bucket) {
        try {
            s3.headBucket(b -> b.bucket(bucket));
        } catch (software.amazon.awssdk.services.s3.model.NoSuchBucketException e) {
            s3.createBucket(b -> b.bucket(bucket));
        } catch (software.amazon.awssdk.services.s3.model.S3Exception e) {
            if (e.statusCode() == 404) {
                s3.createBucket(b -> b.bucket(bucket));
            } else {
                throw e;
            }
        }
    }

    @Test
    @DisplayName("first upload is stored and comes back byte-identical")
    void uploadThenDownload() {
        byte[] blob = "world_state:{Titus.Spared=true}".getBytes(StandardCharsets.UTF_8);

        SaveUploadResponse response = saveService.upload(playerId, 1,
                request("device-pc-01", Map.of(), blob, "EP012", "S1", 100f, 100f, HandPhase.INTACT));

        assertThat(response.headVersion()).isEqualTo(1L);
        assertThat(response.outcome().becomesHead()).isTrue();

        var downloaded = saveService.download(playerId, 1, true);

        assertThat(Base64.getDecoder().decode(downloaded.blobBase64())).isEqualTo(blob);
        assertThat(downloaded.summary().episodeId()).isEqualTo("EP012");
    }

    @Test
    @DisplayName("a second device that never synced produces a conflict, and nothing is lost")
    void divergentDevicesConflictWithoutDataLoss() {
        // PC plays up to EP023.
        byte[] pcBlob = "pc-save-ep023".getBytes(StandardCharsets.UTF_8);
        SaveUploadResponse pcUpload = saveService.upload(playerId, 1,
                request("device-pc-01", Map.of(), pcBlob, "EP023", "S2", 42f, 55f, HandPhase.INTACT));

        assertThat(pcUpload.outcome().isConflict()).isFalse();

        // The PS5 was offline the whole time: it still believes the slot is empty
        // and uploads its own EP021 state.
        byte[] ps5Blob = "ps5-save-ep021".getBytes(StandardCharsets.UTF_8);
        SaveUploadResponse ps5Upload = saveService.upload(playerId, 1,
                request("device-ps5-02", Map.of(), ps5Blob, "EP021", "S2", 60f, 100f, HandPhase.INTACT));

        assertThat(ps5Upload.outcome().isConflict()).isTrue();
        // The PC save is further along, so it keeps the head.
        assertThat(ps5Upload.outcome()).isEqualTo(
                com.fayzinc.ertugrul.save.ConflictResolver.Outcome.CONFLICT_HEAD_WINS);

        // The losing save must still be restorable — this is the whole point.
        assertThat(ps5Upload.conflict()).isNotNull();
        assertThat(ps5Upload.conflict().restorable()).isTrue();

        SaveSlotSummary restored = saveService.restoreConflictCopy(playerId, 1);
        assertThat(restored.hasConflict()).isTrue();

        var afterRestore = saveService.download(playerId, 1, true);
        assertThat(Base64.getDecoder().decode(afterRestore.blobBase64())).isEqualTo(ps5Blob);
    }

    @Test
    @DisplayName("re-uploading identical bytes is a no-op")
    void duplicateUploadIsIdempotent() {
        byte[] blob = "same-bytes".getBytes(StandardCharsets.UTF_8);

        SaveUploadResponse first = saveService.upload(playerId, 2,
                request("device-pc-01", Map.of(), blob, "EP005", "S1", 100f, 100f, HandPhase.INTACT));
        SaveUploadResponse second = saveService.upload(playerId, 2,
                request("device-pc-01", Map.of("device-pc-01", 1L), blob, "EP005", "S1",
                        100f, 100f, HandPhase.INTACT));

        assertThat(second.outcome()).isEqualTo(
                com.fayzinc.ertugrul.save.ConflictResolver.Outcome.ACCEPT_IDENTICAL);
        assertThat(second.headVersion()).isEqualTo(first.headVersion());
    }

    @Test
    @DisplayName("all nine slots are listed, empty ones included")
    void listsEverySlot() {
        saveService.upload(playerId, 0,
                request("device-pc-01", Map.of(), "autosave".getBytes(StandardCharsets.UTF_8),
                        "EP001", "S1", 100f, 100f, HandPhase.INTACT));

        List<SaveSlotSummary> slots = saveService.listSlots(playerId);

        assertThat(slots).hasSize(9);
        assertThat(slots.get(0).slotIndex()).isZero();
        assertThat(slots.get(0).empty()).isFalse();
        assertThat(slots.get(5).empty()).isTrue();
    }

    // ── fixtures ────────────────────────────────────────────────────────────

    private SaveUploadRequest request(String deviceId, Map<String, Long> clock, byte[] blob,
                                      String episodeId, String seasonId,
                                      float handIntegrity, float maxIntegrity, HandPhase phase) {
        return new SaveUploadRequest(
                deviceId,
                clock,
                Base64.getEncoder().encodeToString(blob),
                saveSigner.sha256Hex(blob),
                null,
                Instant.now(),
                new WorldStateSummary(
                        episodeId, seasonId, 3_600, handIntegrity, maxIntegrity, phase,
                        50f, 50f, Map.of(), Map.of("Titus.Spared", "true"),
                        DifficultyTier.ALP, 1));
    }
}
