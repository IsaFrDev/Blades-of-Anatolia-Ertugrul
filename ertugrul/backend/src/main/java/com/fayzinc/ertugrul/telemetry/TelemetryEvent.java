package com.fayzinc.ertugrul.telemetry;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fayzinc.ertugrul.save.DifficultyTier;
import com.fayzinc.ertugrul.save.HandPhase;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.Valid;
import jakarta.validation.constraints.DecimalMax;
import jakarta.validation.constraints.DecimalMin;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.PositiveOrZero;
import jakarta.validation.constraints.Size;

import java.time.Instant;
import java.util.Map;
import java.util.UUID;

/**
 * A single telemetry event.
 *
 * <p><b>Sxema dizayni.</b> Umumiy maydonlar (kim, qayerda, qachon) tekis
 * yotadi, ikkita <i>muhim</i> hodisa turi esa o'z tipli bloklariga ega:
 * {@link WoundState} va {@link Choice}. Qolgani uchun erkin
 * {@code attributes} lug'ati bor.
 *
 * <p>Nega hamma narsani lug'atga solmaymiz: {@code WOUND_STATE} — o'yin
 * balansini boshqaradigan hodisa, va uning maydonlarini tipsiz qoldirish
 * degani, klientdagi bitta imlo xatosi butun balans telemetriyasini jimgina
 * o'chirib qo'yishi mumkin degani. Nega hammasini tiplashtirmaymiz: qolgan
 * hodisalar tez o'zgaradi va har o'zgarishda server deploy'i talab qilinishi
 * — analitikani sekinlashtiradi.
 *
 * @param eventId        client-generated id; the dedupe key for at-least-once delivery
 * @param type           what happened
 * @param playerId       filled in server-side; the client never asserts its own identity
 * @param deviceId       originating device
 * @param sessionId      one play session
 * @param episodeId      EP001..EP048, when the event belongs to an episode
 * @param seasonId       S1..S4
 * @param difficultyTier the tier this playthrough runs at; every metric is sliced by it
 * @param appVersion     client build, for release-over-release comparison
 * @param occurredAt     when the client says it happened
 * @param receivedAt     when the server accepted it; set server-side
 * @param wound          populated for {@code WOUND_STATE}
 * @param choice         populated for {@code CHOICE_MADE}
 * @param attributes     free-form extras for the remaining event types
 */
@JsonInclude(JsonInclude.Include.NON_NULL)
@Schema(name = "TelemetryEvent", description = "One telemetry event")
public record TelemetryEvent(

        @NotNull
        UUID eventId,

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull
        Type type,

        @Schema(description = "Set server-side from the access token; ignored if the client sends it")
        UUID playerId,

        @Size(max = 64)
        String deviceId,

        UUID sessionId,

        @Schema(example = "EP029")
        @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
        String episodeId,

        @Schema(example = "S3")
        @Pattern(regexp = "^S[1-4]$", message = "seasonId must be S1..S4")
        String seasonId,

        DifficultyTier difficultyTier,

        @Size(max = 32)
        String appVersion,

        @NotNull
        Instant occurredAt,

        Instant receivedAt,

        @Valid
        WoundState wound,

        @Valid
        Choice choice,

        @Size(max = 24, message = "at most 24 free-form attributes")
        Map<String, String> attributes
) {

    /** The event taxonomy. Adding a value is safe; renaming one breaks the warehouse. */
    public enum Type {
        EPISODE_START,
        EPISODE_COMPLETE,
        /** Player quit mid-episode — the funnel's drop-off signal. */
        EPISODE_ABANDON,
        DEATH,
        CHOICE_MADE,
        CODEX_UNLOCK,
        /** The wound system's balance probe. Carries {@link WoundState}. */
        WOUND_STATE,
        SETTINGS_CHANGED,
        /** Playable intros are a headline feature; a skip spike means one is not earning its 4 minutes. */
        INTRO_SKIPPED
    }

    /**
     * The wound system's state at a point in time — 05_MIH_SYSTEM.md §8.
     *
     * <p>Bu blok butun balans monitoringining asosi. Undan
     * {@link com.fayzinc.ertugrul.telemetry.funnel.EpisodeFunnelService}
     * epizod juda qattiqmi yoki yo'qligini avtomatik aniqlaydi.
     *
     * @param handIntegrity         current hand condition, 0..100
     * @param maxIntegrity          the ceiling; 100 pre-EP024, 55 after, 70 post-prosthesis
     * @param sabr                  patience, 0..100 — the counter-system to flashbacks
     * @param phase                 INTACT / FRESH / CHRONIC / ADAPTED
     * @param flashbacksThisSession PTSD triggers fired this session
     * @param opiumUsesTotal        lifetime opium uses; every 5th costs 5 permanent ceiling
     * @param deathsThisEpisode     deaths in the current episode
     */
    @Schema(name = "WoundState", description = "HandIntegrity/Sabr probe driving live-ops balance")
    public record WoundState(

            @DecimalMin("0.0") @DecimalMax("100.0")
            float handIntegrity,

            @DecimalMin("0.0") @DecimalMax("100.0")
            float maxIntegrity,

            @DecimalMin("0.0") @DecimalMax("100.0")
            float sabr,

            @NotNull
            HandPhase phase,

            @PositiveOrZero
            int flashbacksThisSession,

            @PositiveOrZero
            int opiumUsesTotal,

            @PositiveOrZero
            int deathsThisEpisode
    ) {
    }

    /**
     * A narrative choice.
     *
     * <p>{@code SS_1}..{@code SS_7} — "Shubha sahnalari": tarixchilar rozi
     * bo'lmagan 7 ta lahza. Ular alohida bayroq bilan belgilanadi, chunki
     * ularning statistikasi o'yinchiga boshqacha ko'rsatiladi — olimlar
     * fikri bilan birga.
     *
     * @param choiceId          choice identifier, or SS_1..SS_7 for an uncertainty scene
     * @param optionId          which option was taken
     * @param uncertaintyScene  true for SS_1..SS_7
     * @param deliberationMs    how long the player sat on it; a proxy for how hard it felt
     */
    @Schema(name = "Choice", description = "A narrative choice, including the 7 uncertainty scenes")
    public record Choice(

            @Schema(example = "SS_1")
            @Size(max = 64)
            String choiceId,

            @Schema(example = "GUNDUZ_ALP")
            @Size(max = 64)
            String optionId,

            boolean uncertaintyScene,

            @PositiveOrZero
            long deliberationMs
    ) {
    }

    /** Regex matching the seven uncertainty scenes. */
    public static final String UNCERTAINTY_SCENE_PATTERN = "^SS_[1-7]$";

    public TelemetryEvent {
        attributes = attributes == null ? Map.of() : Map.copyOf(attributes);
    }

    /**
     * Serverdagi haqiqiy identifikatorlar bilan to'ldirilgan nusxa qaytaradi.
     *
     * <p>Klient yuborgan {@code playerId} ga hech qachon ishonilmaydi — u
     * har doim access token'dan olinadi. Aks holda har kim boshqa o'yinchi
     * nomidan telemetriya yuborib, statistikani buzishi mumkin edi.
     *
     * @param authenticatedPlayerId the id from the access token
     * @return a copy stamped with the trusted player id and server receive time
     */
    public TelemetryEvent stamped(UUID authenticatedPlayerId) {
        return new TelemetryEvent(
                eventId, type, authenticatedPlayerId, deviceId, sessionId,
                episodeId, seasonId, difficultyTier, appVersion,
                occurredAt, Instant.now(), wound, choice, attributes);
    }

    /**
     * Pseudonymised copy for players whose telemetry consent is ANONYMOUS.
     *
     * <p>Player id o'rniga barqaror, lekin qaytarib bo'lmaydigan pseudonym
     * qo'yiladi: hodisalar bir sessiya ichida bog'lanib qoladi (funnel
     * ishlashi uchun bu shart), lekin shaxsga olib bormaydi.
     *
     * @param pseudonym rotating, non-reversible identifier
     */
    public TelemetryEvent pseudonymised(UUID pseudonym) {
        return new TelemetryEvent(
                eventId, type, pseudonym, null, sessionId,
                episodeId, seasonId, difficultyTier, appVersion,
                occurredAt, Instant.now(), wound, choice, attributes);
    }

    /** Episode number, or 0 when the event is not tied to an episode. */
    public int episodeNumber() {
        if (episodeId == null || episodeId.length() != 5) {
            return 0;
        }
        try {
            return Integer.parseInt(episodeId.substring(2));
        } catch (NumberFormatException e) {
            return 0;
        }
    }
}
