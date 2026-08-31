package com.fayzinc.ertugrul.save.dto;

import com.fayzinc.ertugrul.save.SaveSlot;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.Map;

/**
 * One row of the "Continue" screen.
 *
 * <p>Bu javob S3'ga umuman bormaydi — barcha maydonlar Postgres'dagi
 * denormalizatsiya qilingan xulosadan olinadi. Shuning uchun 9 ta slotni
 * ko'rsatish bitta so'rov va bir necha millisekund.
 *
 * @param slotIndex      0 = autosave, 1..8 = manual
 * @param empty          true when nothing has ever been saved here
 * @param headVersion    current version number
 * @param vectorClock    the head's clock; the client sends it back on the next upload
 * @param episodeId      EP001..EP048
 * @param seasonId       S1..S4
 * @param playtimeSeconds total play time
 * @param handIntegrity  current hand condition; the client picks a HUD icon from it
 * @param maxIntegrity   the ceiling
 * @param difficultyTier difficulty this playthrough runs at
 * @param schemaVersion  blob layout version; the client refuses newer than it knows
 * @param hasConflict    true when a conflict copy is waiting to be resolved
 * @param updatedAt      when the head was written
 */
@Schema(name = "SaveSlotSummary", description = "One slot as shown on the Continue screen")
public record SaveSlotSummary(

        int slotIndex,
        boolean empty,
        long headVersion,
        Map<String, Long> vectorClock,

        String episodeId,
        String seasonId,
        long playtimeSeconds,

        @Schema(description = "Never rendered as a number in-game; drives the hand icon")
        float handIntegrity,
        float maxIntegrity,

        String difficultyTier,
        int schemaVersion,

        @Schema(description = "True when another device's save is retained and restorable")
        boolean hasConflict,

        Instant updatedAt
) {

    public static SaveSlotSummary from(SaveSlot slot) {
        return new SaveSlotSummary(
                slot.getSlotIndex(),
                slot.isEmpty(),
                slot.getHeadVersion(),
                slot.getVectorClock(),
                slot.getEpisodeId(),
                slot.getSeasonId(),
                slot.getPlaytimeSeconds(),
                slot.getHandIntegrity(),
                slot.getMaxIntegrity(),
                slot.getDifficultyTier(),
                slot.getSchemaVersion(),
                slot.isHasConflict(),
                slot.getUpdatedAt());
    }

    /** Placeholder row for a slot that has never been written. */
    public static SaveSlotSummary emptySlot(int slotIndex) {
        return new SaveSlotSummary(slotIndex, true, 0, Map.of(),
                null, null, 0, 100f, 100f, null, 1, false, null);
    }
}
