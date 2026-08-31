package com.fayzinc.ertugrul.journey.dto;

import com.fayzinc.ertugrul.journey.JourneyEntry;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.PositiveOrZero;
import jakarta.validation.constraints.Size;

import java.time.LocalDate;
import java.util.List;
import java.util.UUID;

/**
 * A diary page written by the client.
 *
 * <p>Matn klientda tug'iladi — o'yin uni o'yinchining tanlovlaridan,
 * uchrashgan odamlaridan va ochgan kodekslaridan shablon orqali yozadi.
 * Server matnni <i>yaratmaydi</i>: u o'yin dunyosining to'liq holatini
 * bilmaydi va bilishi ham kerak emas.
 *
 * <p>Idempotent: {@code (playthroughId, sequenceNo)} juftligi unikal, shuning
 * uchun tarmoq uzilishidan keyin qayta yuborish xavfsiz.
 *
 * @param playthroughId     which diary; NG+ starts a new one
 * @param sequenceNo        page order within the diary; gaps are fine
 * @param episodeId         EP001..EP048
 * @param seasonId          S1..S4
 * @param hijriDateText     e.g. {@code 632 Rabi al-awwal}
 * @param gregorianDateText e.g. {@code 1234 December}
 * @param inGameDate        sortable anchor for the timeline
 * @param placeName         where the entry was written
 * @param body              the diary text, in the player's voice
 * @param linkedCodexIds    codex references shown at the foot of the page
 * @param tone              emotional colour, driving the PDF's ink and border
 * @param writtenLeftHanded true after EP024; renders in an unsteady script
 * @param deviceId          which device wrote it
 */
@Schema(name = "JourneyEntryRequest", description = "A Safar Daftari page written by the client")
public record JourneyEntryRequest(

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        UUID playthroughId,

        @PositiveOrZero
        int sequenceNo,

        @Schema(example = "EP024", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
        String episodeId,

        @Schema(example = "S2", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Pattern(regexp = "^S[1-4]$", message = "seasonId must be S1..S4")
        String seasonId,

        @Schema(example = "632 Rabi al-awwal", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Size(max = 64)
        String hijriDateText,

        @Schema(example = "1234 December", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Size(max = 64)
        String gregorianDateText,

        @NotNull
        LocalDate inGameDate,

        @Schema(example = "Sultan Han")
        @Size(max = 96)
        String placeName,

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Size(min = 1, max = 8000, message = "body must be 1..8000 characters")
        String body,

        @Size(max = 24, message = "at most 24 codex links per page")
        List<@Pattern(regexp = "^CDX_[A-Z0-9_]{2,48}$") String> linkedCodexIds,

        JourneyEntry.Tone tone,

        boolean writtenLeftHanded,

        @Size(max = 64)
        String deviceId
) {

    public JourneyEntryRequest {
        linkedCodexIds = linkedCodexIds == null ? List.of() : List.copyOf(linkedCodexIds);
        tone = tone == null ? JourneyEntry.Tone.NEUTRAL : tone;
    }
}
