package com.fayzinc.ertugrul.save.dto;

import com.fayzinc.ertugrul.save.ConflictResolver;
import io.swagger.v3.oas.annotations.media.Schema;

import java.util.Map;

/**
 * The outcome of an upload.
 *
 * <p>Klient aynan {@code outcome} bo'yicha qaror qabul qiladi:
 * <ul>
 *   <li>{@code ACCEPT_*} — jimgina davom etadi, o'yinchi hech narsa
 *       sezmaydi;</li>
 *   <li>{@code REJECT_STALE} — jimgina serverdan tortib oladi (o'yinchini
 *       bezovta qilmaydi, chunki u shunchaki boshqa qurilmada o'ynagan);</li>
 *   <li>{@code CONFLICT_*} — <b>faqat shu holatda</b> o'yinchiga oyna
 *       ko'rsatiladi: "boshqa qurilmada boshqa saqlash bor".</li>
 * </ul>
 *
 * @param outcome        what the server did
 * @param headVersion    the slot's version after this call
 * @param vectorClock    the clock the client must carry forward
 * @param conflict       populated only when {@code outcome.isConflict()}
 * @param message        human-readable reason, for logs and QA
 */
@Schema(name = "SaveUploadResponse", description = "Result of a save upload")
public record SaveUploadResponse(

        ConflictResolver.Outcome outcome,
        long headVersion,
        Map<String, Long> vectorClock,

        @Schema(description = "Present only on a conflict; describes the save the player may restore")
        ConflictReport conflict,

        String message
) {

    /**
     * Describes the losing save so the client can render a meaningful choice.
     *
     * <p>Muhim: o'yinchiga "versiya 14 va versiya 15" deb ko'rsatilmaydi — bu
     * hech narsa anglatmaydi. Unga <b>epizod, o'ynalgan vaqt va qurilma</b>
     * ko'rsatiladi: "PS5 dagi saqlash — EP021, 9 soat 12 daqiqa".
     *
     * @param retainedVersion   version number of the retained save
     * @param episodeId         where that save was
     * @param playtimeSeconds   how far it had progressed
     * @param originDeviceId    which device wrote it
     * @param handIntegrity     hand condition in the retained save
     * @param restorable        whether the client may offer a one-tap restore
     */
    @Schema(name = "ConflictReport")
    public record ConflictReport(
            long retainedVersion,
            String episodeId,
            long playtimeSeconds,
            String originDeviceId,
            float handIntegrity,
            boolean restorable
    ) {
    }

    public static SaveUploadResponse accepted(ConflictResolver.Outcome outcome,
                                              long headVersion,
                                              Map<String, Long> clock,
                                              String message) {
        return new SaveUploadResponse(outcome, headVersion, clock, null, message);
    }

    public static SaveUploadResponse conflicted(ConflictResolver.Outcome outcome,
                                                long headVersion,
                                                Map<String, Long> clock,
                                                ConflictReport report,
                                                String message) {
        return new SaveUploadResponse(outcome, headVersion, clock, report, message);
    }
}
