package com.fayzinc.ertugrul.codex.dto;

import com.fayzinc.ertugrul.codex.CodexCategory;
import com.fayzinc.ertugrul.codex.CodexEntryProgress;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.PositiveOrZero;

import java.time.Instant;

/**
 * One unlocked codex entry, as sent by the client or returned by the server.
 *
 * <p>Bir xil tur ikki yo'nalishda ham ishlatiladi — bu ataylab: sinxron
 * <i>simmetrik</i>, ya'ni klient yuboradigan va oladigan narsa bir xil
 * shaklda. Assimetrik DTO'lar bu yerda faqat ikki marta xato qilish imkonini
 * berardi.
 *
 * @param codexId      e.g. {@code CDX_KAYI_TRIBE}
 * @param confidence   DOCUMENTED / DISPUTED / LEGEND
 * @param category     which of the eight categories
 * @param unlockMethod OBSERVE / USE / DIALOGUE / FIND / EVENT
 * @param episodeId    where it was unlocked, EP001..EP048
 * @param unlockedAt   when; the earliest value across devices wins
 * @param readCount    times opened; the maximum across devices wins
 * @param bookmarked   player bookmark; latest revision wins
 * @param revision     LWW counter for the mutable fields
 */
@Schema(name = "CodexUnlock", description = "A single codex unlock, symmetric in both sync directions")
public record CodexUnlockDto(

        @Schema(example = "CDX_KAYI_TRIBE", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Pattern(regexp = "^CDX_[A-Z0-9_]{2,48}$", message = "codexId must look like CDX_SOME_ENTRY")
        String codexId,

        @Schema(example = "LEGEND", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        CodexEntryProgress.Confidence confidence,

        @Schema(example = "SOCIETY", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        CodexCategory category,

        @Schema(example = "OBSERVE", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        CodexEntryProgress.UnlockMethod unlockMethod,

        @Schema(example = "EP001")
        @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
        String episodeId,

        @NotNull
        Instant unlockedAt,

        @PositiveOrZero
        int readCount,

        boolean bookmarked,

        @PositiveOrZero
        long revision
) {

    public static CodexUnlockDto from(CodexEntryProgress entity) {
        return new CodexUnlockDto(
                entity.getCodexId(),
                entity.getConfidence(),
                entity.getCategory(),
                entity.getUnlockMethod(),
                entity.getEpisodeId(),
                entity.getUnlockedAt(),
                entity.getReadCount(),
                entity.isBookmarked(),
                entity.getRevision());
    }
}
