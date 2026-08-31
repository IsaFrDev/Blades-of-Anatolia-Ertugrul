package com.fayzinc.ertugrul.journey.dto;

import com.fayzinc.ertugrul.journey.JourneyExport;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.UUID;

/**
 * The state of a PDF/PNG export.
 *
 * <p>Klient buni so'rab turadi. {@code READY} bo'lgach {@code downloadUrl}
 * to'ldiriladi — u vaqtinchalik imzolangan havola, ya'ni fayl backend orqali
 * oqib o'tmaydi.
 *
 * @param exportId      the job
 * @param status        PENDING / RUNNING / READY / FAILED / EXPIRED
 * @param format        PDF or PNG
 * @param downloadUrl   presigned URL; present only when READY
 * @param entryCount    how many diary pages were rendered
 * @param sizeBytes     file size
 * @param failureReason short explanation when FAILED
 * @param requestedAt   when the export was asked for
 * @param expiresAt     when the generated file is deleted
 */
@Schema(name = "ExportStatus", description = "State of a Safar Daftari export job")
public record ExportStatusResponse(

        UUID exportId,
        JourneyExport.Status status,
        JourneyExport.Format format,

        @Schema(description = "Presigned URL; present only when status is READY")
        String downloadUrl,

        Integer entryCount,
        Integer sizeBytes,
        String failureReason,
        Instant requestedAt,
        Instant expiresAt
) {

    public static ExportStatusResponse from(JourneyExport export, String downloadUrl) {
        return new ExportStatusResponse(
                export.getId(),
                export.getStatus(),
                export.getFormat(),
                downloadUrl,
                export.getEntryCount(),
                export.getSizeBytes(),
                export.getFailureReason(),
                export.getRequestedAt(),
                export.getExpiresAt());
    }
}
