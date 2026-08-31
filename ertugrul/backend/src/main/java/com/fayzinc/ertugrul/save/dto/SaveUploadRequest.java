package com.fayzinc.ertugrul.save.dto;

import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

import java.time.Instant;
import java.util.Map;

/**
 * A cloud-save upload.
 *
 * <p>Blob base64 sifatida yuboriladi. Bu ataylab: 8 MiB gacha bo'lgan bitta
 * blob uchun multipart murakkabligi o'zini oqlamaydi, va JSON tanasi klientning
 * (UE5) HTTP qatlami bilan eng sodda ishlaydigan variant. Katta blob'lar uchun
 * kelajakda presigned PUT yo'li ochiq — u holda bu endpoint faqat metadata
 * qabul qiladi.
 *
 * @param deviceId       the writing device; also the vector-clock node key
 * @param vectorClock    the client's clock BEFORE this write; the server increments it
 * @param blobBase64     the opaque save payload
 * @param sha256         client-computed content hash, verified server-side
 * @param clientHmac     optional client signature; a soft anti-cheat signal only
 * @param clientSavedAt  the client's claimed save time; LWW tiebreak input only
 * @param summary        readable state for the load screen and sanity checks
 */
@Schema(name = "SaveUploadRequest", description = "Upload a save blob to a slot")
public record SaveUploadRequest(

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Size(min = 8, max = 64)
        String deviceId,

        @Schema(description = "Vector clock as seen by the client before this write",
                example = "{\"device-pc-01\": 12, \"device-ps5-02\": 4}")
        Map<String, Long> vectorClock,

        @Schema(description = "Base64-encoded save blob", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        String blobBase64,

        @Schema(example = "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
                requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Pattern(regexp = "^[0-9a-f]{64}$", message = "sha256 must be 64 lowercase hex characters")
        String sha256,

        @Schema(description = "Optional client HMAC; mismatch raises suspicion but never rejects")
        @Pattern(regexp = "^[0-9a-f]{64}$", message = "clientHmac must be 64 lowercase hex characters")
        String clientHmac,

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        Instant clientSavedAt,

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        @Valid
        WorldStateSummary summary
) {

    public SaveUploadRequest {
        vectorClock = vectorClock == null ? Map.of() : Map.copyOf(vectorClock);
    }
}
