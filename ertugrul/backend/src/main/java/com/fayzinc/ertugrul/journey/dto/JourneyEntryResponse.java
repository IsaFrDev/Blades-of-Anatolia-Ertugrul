package com.fayzinc.ertugrul.journey.dto;

import com.fayzinc.ertugrul.journey.JourneyEntry;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.time.LocalDate;
import java.util.List;
import java.util.UUID;

/**
 * A diary page as returned to the owning player.
 *
 * <p>Ommaviy havola uchun <b>boshqa</b> tur ishlatiladi
 * ({@link PublicJourneyResponse}) — u {@code playthroughId} va boshqa ichki
 * identifikatorlarni umuman o'z ichiga olmaydi. Bir turni ikki maqsadda
 * ishlatish — maxfiy maydonni tasodifan ommaviy qilib yuborishning eng
 * keng tarqalgan yo'li.
 *
 * @param id                page id
 * @param playthroughId     which diary
 * @param sequenceNo        order within the diary
 * @param episodeId         EP001..EP048
 * @param seasonId          S1..S4
 * @param dualDateHeading   "632 Rabi al-awwal / 1234 December"
 * @param inGameDate        sortable anchor
 * @param placeName         where it was written
 * @param body              the diary text
 * @param linkedCodexIds    codex references
 * @param tone              emotional colour
 * @param writtenLeftHanded true for post-EP024 pages
 * @param hiddenFromShare   whether the player excluded it from shared links
 * @param createdAt         when it was written
 */
@Schema(name = "JourneyEntryResponse", description = "A diary page, for the owning player")
public record JourneyEntryResponse(

        UUID id,
        UUID playthroughId,
        int sequenceNo,
        String episodeId,
        String seasonId,
        String dualDateHeading,
        LocalDate inGameDate,
        String placeName,
        String body,
        List<String> linkedCodexIds,
        JourneyEntry.Tone tone,
        boolean writtenLeftHanded,
        boolean hiddenFromShare,
        Instant createdAt
) {

    public static JourneyEntryResponse from(JourneyEntry entry) {
        return new JourneyEntryResponse(
                entry.getId(),
                entry.getPlaythroughId(),
                entry.getSequenceNo(),
                entry.getEpisodeId(),
                entry.getSeasonId(),
                entry.dualDateHeading(),
                entry.getInGameDate(),
                entry.getPlaceName(),
                entry.getBody(),
                entry.getLinkedCodexIds(),
                entry.getTone(),
                entry.isWrittenLeftHanded(),
                entry.isHiddenFromShare(),
                entry.getCreatedAt());
    }
}
