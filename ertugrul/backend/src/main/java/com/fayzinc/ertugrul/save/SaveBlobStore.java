package com.fayzinc.ertugrul.save;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;
import software.amazon.awssdk.core.ResponseBytes;
import software.amazon.awssdk.core.sync.RequestBody;
import software.amazon.awssdk.services.s3.S3Client;
import software.amazon.awssdk.services.s3.model.Delete;
import software.amazon.awssdk.services.s3.model.DeleteObjectsRequest;
import software.amazon.awssdk.services.s3.model.GetObjectRequest;
import software.amazon.awssdk.services.s3.model.GetObjectResponse;
import software.amazon.awssdk.services.s3.model.NoSuchKeyException;
import software.amazon.awssdk.services.s3.model.ObjectIdentifier;
import software.amazon.awssdk.services.s3.model.PutObjectRequest;
import software.amazon.awssdk.services.s3.model.S3Exception;
import software.amazon.awssdk.services.s3.presigner.S3Presigner;
import software.amazon.awssdk.services.s3.presigner.model.GetObjectPresignRequest;

import java.net.URL;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Reads and writes save blobs in the object store.
 *
 * <p><b>Nega blob'lar Postgres'da emas.</b> Bitta blob 8 MiB gacha. Millionlab
 * o'yinchi, har birida 9 slot, har slotda 10 versiya tarixi — bu Postgres
 * uchun to'g'ri yuk emas: WAL shishadi, backup soatlab cho'ziladi, va eng
 * yomoni — blob trafigi <i>metadata</i> so'rovlarini sekinlashtiradi. Object
 * store bu ish uchun yaratilgan; Postgres esa faqat kichik, indekslangan
 * metadata bilan qoladi.
 *
 * <p>Kalit sxemasi: {@code saves/{playerId}/{slotIndex}/{version}.sav} —
 * o'zgarmas, ya'ni har yuklash yangi obyekt. Bu tarixni bepul beradi va
 * "yozish paytida o'qish" poyga holatini butunlay yo'q qiladi.
 */
@Component
public class SaveBlobStore {

    private static final Logger log = LoggerFactory.getLogger(SaveBlobStore.class);

    private static final String CONTENT_TYPE = "application/octet-stream";

    private final S3Client s3;
    private final S3Presigner presigner;
    private final ErtugrulProperties props;

    public SaveBlobStore(S3Client s3, S3Presigner presigner, ErtugrulProperties props) {
        this.s3 = s3;
        this.presigner = presigner;
        this.props = props;
    }

    /**
     * Save blob uchun obyekt kalitini quradi.
     *
     * @param playerId  owning player
     * @param slotIndex 0 (autosave) or 1..8
     * @param version   monotonic version within the slot
     * @return the immutable object key
     */
    public String objectKey(UUID playerId, int slotIndex, long version) {
        return "saves/%s/%d/%d.sav".formatted(playerId, slotIndex, version);
    }

    /**
     * Blob'ni object store'ga yozadi.
     *
     * <p>Metadata sifatida sha256 va HMAC ham yoziladi. Bu takrorlash emas —
     * agar Postgres va object store bir-biridan ajralib qolsa (masalan,
     * tiklashdan keyin), obyektning o'zidan kim ekanini aniqlash mumkin bo'ladi.
     *
     * @param objectKey  where to write
     * @param blob       raw bytes
     * @param sha256     content hash, stored as object metadata
     * @param serverHmac server signature, stored as object metadata
     * @throws ErtugrulException {@code STORAGE_UNAVAILABLE} when the store rejects the write
     */
    public void put(String objectKey, byte[] blob, String sha256, String serverHmac) {
        try {
            s3.putObject(PutObjectRequest.builder()
                            .bucket(props.s3().saveBucket())
                            .key(objectKey)
                            .contentType(CONTENT_TYPE)
                            .contentLength((long) blob.length)
                            .metadata(Map.of(
                                    "sha256", sha256,
                                    "hmac", serverHmac))
                            .build(),
                    RequestBody.fromBytes(blob));

            log.debug("Stored save blob key={} bytes={}", objectKey, blob.length);

        } catch (S3Exception e) {
            log.error("Failed to store save blob key={}", objectKey, e);
            throw new ErtugrulException(ErtugrulException.Code.STORAGE_UNAVAILABLE,
                    "Could not write save blob", e);
        }
    }

    /**
     * Blob'ni o'qiydi.
     *
     * @param objectKey the key to read
     * @return raw bytes
     * @throws ErtugrulException {@code SAVE_NOT_FOUND} when the object is missing
     */
    public byte[] get(String objectKey) {
        try {
            ResponseBytes<GetObjectResponse> response = s3.getObjectAsBytes(
                    GetObjectRequest.builder()
                            .bucket(props.s3().saveBucket())
                            .key(objectKey)
                            .build());
            return response.asByteArray();

        } catch (NoSuchKeyException e) {
            // Metadata says the version exists but the blob does not: either a
            // half-finished upload or store corruption. Either way the client
            // must not be handed garbage.
            log.error("Save blob missing from store: key={}", objectKey);
            throw new ErtugrulException(ErtugrulException.Code.SAVE_NOT_FOUND,
                    "Save blob is missing from storage", e);

        } catch (S3Exception e) {
            log.error("Failed to read save blob key={}", objectKey, e);
            throw new ErtugrulException(ErtugrulException.Code.STORAGE_UNAVAILABLE,
                    "Could not read save blob", e);
        }
    }

    /**
     * To'g'ridan-to'g'ri yuklab olish uchun vaqtinchalik imzolangan URL beradi.
     *
     * <p>8 MiB blob backend orqali oqib o'tishi shart emas — klient object
     * store bilan bevosita gaplashadi. Bu servisning eng katta trafik
     * tejamkorligi: backend faqat metadata va imzo bilan shug'ullanadi.
     *
     * @param objectKey the blob to expose
     * @return a URL valid for {@code ertugrul.s3.presign-ttl}
     */
    public URL presignedDownloadUrl(String objectKey) {
        GetObjectPresignRequest request = GetObjectPresignRequest.builder()
                .signatureDuration(props.s3().presignTtl())
                .getObjectRequest(GetObjectRequest.builder()
                        .bucket(props.s3().saveBucket())
                        .key(objectKey)
                        .build())
                .build();

        return presigner.presignGetObject(request).url();
    }

    /**
     * Bir nechta blob'ni o'chiradi — tungi tozalash va GDPR uchun.
     *
     * <p>Xatolik yuz bersa istisno tashlanmaydi: tozalash <i>eng yaxshi
     * harakat</i> (best-effort) ishi. Qolib ketgan obyekt keyingi yurishda
     * yana urinib ko'riladi; butun jobni to'xtatish esa navbatni o'stiradi.
     *
     * @param objectKeys keys to remove
     * @return number of keys the store reported as deleted
     */
    public int deleteAll(List<String> objectKeys) {
        if (objectKeys == null || objectKeys.isEmpty()) {
            return 0;
        }

        List<ObjectIdentifier> identifiers = objectKeys.stream()
                .map(key -> ObjectIdentifier.builder().key(key).build())
                .toList();

        try {
            var response = s3.deleteObjects(DeleteObjectsRequest.builder()
                    .bucket(props.s3().saveBucket())
                    .delete(Delete.builder().objects(identifiers).quiet(true).build())
                    .build());

            int errors = response.errors() == null ? 0 : response.errors().size();
            if (errors > 0) {
                log.warn("Blob cleanup: {} of {} keys failed to delete", errors, objectKeys.size());
            }
            return objectKeys.size() - errors;

        } catch (S3Exception e) {
            log.warn("Blob cleanup batch failed for {} keys; will retry next run", objectKeys.size(), e);
            return 0;
        }
    }
}
