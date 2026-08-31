package com.fayzinc.ertugrul.journey.dto;

import com.fayzinc.ertugrul.journey.JourneyEntry;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.List;

/**
 * A shared diary, as seen by anyone with the link.
 *
 * <p><b>Bu tur ataylab kambag'al.</b> Unda o'yinchi ID'si, playthrough ID'si,
 * qurilma ID'si, sana belgilar — hech biri yo'q. Faqat o'yinchi ulashishni
 * <i>tanlagan</i> narsa: sarlavha, sahifalar matni va sanalari.
 *
 * <p>{@link JourneyEntryResponse} dan alohida tur bo'lishi shart: bitta turni
 * ikkala maqsadda ishlatish — kelajakda qo'shiladigan yangi maydon jimgina
 * ommaviy bo'lib qolishining eng oson yo'li.
 *
 * @param title       what the player called this diary, or a neutral default
 * @param entryCount  how many pages are shared
 * @param pages       the shared pages, in order
 * @param sharedAt    when the link was created
 */
@Schema(name = "PublicJourney", description = "A shared diary; contains no identifying data")
public record PublicJourneyResponse(

        @Schema(example = "Ertugrul's road, 1227-1261")
        String title,

        int entryCount,

        List<PublicPage> pages,

        Instant sharedAt
) {

    /**
     * One shared page.
     *
     * @param dualDateHeading   "632 Rabi al-awwal / 1234 December"
     * @param placeName         where it was written
     * @param body              the diary text
     * @param linkedCodexIds    codex references, so the public page can link to the wiki
     * @param tone              emotional colour, for styling
     * @param writtenLeftHanded renders in the unsteady post-EP024 script
     */
    @Schema(name = "PublicPage")
    public record PublicPage(
            String dualDateHeading,
            String placeName,
            String body,
            List<String> linkedCodexIds,
            JourneyEntry.Tone tone,
            boolean writtenLeftHanded
    ) {
        public static PublicPage from(JourneyEntry entry) {
            return new PublicPage(
                    entry.dualDateHeading(),
                    entry.getPlaceName(),
                    entry.getBody(),
                    entry.getLinkedCodexIds(),
                    entry.getTone(),
                    entry.isWrittenLeftHanded());
        }
    }
}
