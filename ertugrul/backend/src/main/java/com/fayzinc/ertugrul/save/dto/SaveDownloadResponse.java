package com.fayzinc.ertugrul.save.dto;

import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.Map;

/**
 * A save handed back to the client.
 *
 * <p>Ikki rejim bor va ikkalasi ham kerak:
 * <ul>
 *   <li><b>Inline</b> ({@code blobBase64} to'ldirilgan) — kichik blob'lar va
 *       konsollar uchun, ular qo'shimcha HTTP klientini ochishni yoqtirmaydi;</li>
 *   <li><b>Presigned URL</b> ({@code downloadUrl} to'ldirilgan) — katta
 *       blob'lar uchun, klient object store bilan bevosita gaplashadi va
 *       backend trafigi tejaladi.</li>
 * </ul>
 * Qaysi biri berilishini server hal qiladi, blob hajmiga qarab.
 *
 * @param slotIndex     which slot
 * @param version       version being returned
 * @param vectorClock   the clock the client must adopt
 * @param blobBase64    inline payload, or null when a URL is used instead
 * @param downloadUrl   presigned URL, or null when the payload is inline
 * @param sha256        content hash so the client can verify what it received
 * @param serverHmac    server signature over the blob identity
 * @param sizeBytes     blob size
 * @param summary       readable state, so the client can decide before downloading
 * @param clientSavedAt the originating client's claimed save time
 */
@Schema(name = "SaveDownloadResponse", description = "A save blob, inline or by presigned URL")
public record SaveDownloadResponse(

        int slotIndex,
        long version,
        Map<String, Long> vectorClock,

        @Schema(description = "Inline payload; null when downloadUrl is set")
        String blobBase64,

        @Schema(description = "Presigned URL; null when the payload is inline")
        String downloadUrl,

        String sha256,
        String serverHmac,
        int sizeBytes,

        WorldStateSummaryView summary,
        Instant clientSavedAt
) {

    /**
     * Lightweight projection of the stored summary.
     *
     * <p>Alohida tur, chunki bu <i>serverda saqlangani</i> — yuklashdagi to'liq
     * {@link WorldStateSummary} emas. Server fraksiya obro'si va dunyo
     * bayroqlarini saqlamaydi (ular blob ichida), shuning uchun ularni
     * qaytarishga ham va'da bermaydi.
     */
    @Schema(name = "WorldStateSummaryView")
    public record WorldStateSummaryView(
            String episodeId,
            String seasonId,
            long playtimeSeconds,
            float handIntegrity,
            float maxIntegrity,
            String difficultyTier,
            int schemaVersion
    ) {
    }
}
