package com.fayzinc.ertugrul.integrity;

import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.save.DifficultyTier;
import com.fayzinc.ertugrul.save.HandPhase;
import com.fayzinc.ertugrul.telemetry.TelemetryEvent;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import java.time.Duration;
import java.time.Instant;
import java.util.Map;
import java.util.UUID;

import static org.assertj.core.api.Assertions.assertThat;

/**
 * Tests the wound system's physical invariants.
 *
 * <p>Bu tekshiruvlar o'yin dizaynidan to'g'ridan-to'g'ri kelib chiqadi
 * (05_MIH_SYSTEM.md) va ularning maqsadi cheat'ni ushlash emas — balans
 * telemetriyasini <b>ifloslanishdan</b> saqlash. Bitta buzilgan klient
 * bizni noto'g'ri live-ops qaroriga olib kelishi mumkin.
 */
class TelemetrySanityCheckerTest {

    private final TelemetrySanityChecker checker =
            new TelemetrySanityChecker(testProperties());

    @Test
    @DisplayName("a normal chronic-phase reading passes")
    void plausibleWoundStatePasses() {
        TelemetryEvent event = woundEvent("EP029", "S3", 23.4f, 55.0f, HandPhase.CHRONIC, 4);

        assertThat(checker.check(event)).isEmpty();
    }

    @Test
    @DisplayName("hand integrity above the ceiling is impossible")
    void integrityAboveCeilingRejected() {
        TelemetryEvent event = woundEvent("EP029", "S3", 80.0f, 55.0f, HandPhase.CHRONIC, 2);

        assertThat(checker.check(event))
                .get()
                .extracting(TelemetrySanityChecker.Rejection::reason)
                .isEqualTo("INTEGRITY_ABOVE_CEILING");
    }

    @Test
    @DisplayName("being wounded before EP024 is impossible — the nail has not been driven")
    void woundedBeforeNailEpisodeRejected() {
        TelemetryEvent event = woundEvent("EP012", "S1", 40.0f, 55.0f, HandPhase.FRESH, 1);

        assertThat(checker.check(event)).isPresent();
        assertThat(checker.check(event).get().indicatesTampering()).isTrue();
    }

    @Test
    @DisplayName("the ceiling cannot exceed 55 after the nail, before the prosthesis")
    void ceilingAbovePhaseMaximumRejected() {
        // EP029 is CHRONIC: the ceiling is 55 and only the EP043 prosthesis lifts it.
        TelemetryEvent event = woundEvent("EP029", "S3", 50.0f, 90.0f, HandPhase.CHRONIC, 1);

        assertThat(checker.check(event))
                .get()
                .extracting(TelemetrySanityChecker.Rejection::reason)
                .isEqualTo("CEILING_ABOVE_PHASE_MAXIMUM");
    }

    @Test
    @DisplayName("the EP043 prosthesis legitimately raises the ceiling to 70")
    void prosthesisCeilingAccepted() {
        TelemetryEvent event = woundEvent("EP045", "S4", 62.0f, 70.0f, HandPhase.ADAPTED, 1);

        assertThat(checker.check(event)).isEmpty();
    }

    @Test
    @DisplayName("episode and season must agree")
    void episodeSeasonMismatchRejected() {
        // EP029 belongs to S3, not S1.
        TelemetryEvent event = woundEvent("EP029", "S1", 30.0f, 55.0f, HandPhase.CHRONIC, 1);

        assertThat(checker.check(event))
                .get()
                .extracting(TelemetrySanityChecker.Rejection::reason)
                .isEqualTo("EPISODE_SEASON_MISMATCH");
    }

    @Test
    @DisplayName("an event from the future is rejected")
    void futureEventRejected() {
        TelemetryEvent event = new TelemetryEvent(
                UUID.randomUUID(), TelemetryEvent.Type.EPISODE_START, null, "dev", UUID.randomUUID(),
                "EP001", "S1", DifficultyTier.ALP, "1.0.0",
                Instant.now().plus(Duration.ofHours(2)), null, null, null, Map.of());

        assertThat(checker.check(event))
                .get()
                .extracting(TelemetrySanityChecker.Rejection::reason)
                .isEqualTo("EVENT_IN_FUTURE");
    }

    @Test
    @DisplayName("a stale offline event is rejected but is NOT treated as tampering")
    void staleEventIsNotTampering() {
        TelemetryEvent event = new TelemetryEvent(
                UUID.randomUUID(), TelemetryEvent.Type.EPISODE_START, null, "dev", UUID.randomUUID(),
                "EP001", "S1", DifficultyTier.ALP, "1.0.0",
                Instant.now().minus(Duration.ofDays(30)), null, null, null, Map.of());

        TelemetrySanityChecker.Rejection rejection = checker.check(event).orElseThrow();

        assertThat(rejection.reason()).isEqualTo("EVENT_TOO_OLD");
        // An honest player who played offline for a month must not be punished.
        assertThat(rejection.indicatesTampering()).isFalse();
    }

    @Test
    @DisplayName("uncertainty scenes must occur in their own episode")
    void uncertaintySceneWrongEpisodeRejected() {
        // SS_1 ("who was your father?") belongs to EP004.
        TelemetryEvent event = choiceEvent("EP012", "S1", "SS_1", "GUNDUZ_ALP", true);

        assertThat(checker.check(event))
                .get()
                .extracting(TelemetrySanityChecker.Rejection::reason)
                .isEqualTo("UNCERTAINTY_SCENE_WRONG_EPISODE");
    }

    @Test
    @DisplayName("SS_1 in EP004 is accepted")
    void uncertaintySceneInCorrectEpisodeAccepted() {
        TelemetryEvent event = choiceEvent("EP004", "S1", "SS_1", "GUNDUZ_ALP", true);

        assertThat(checker.check(event)).isEmpty();
    }

    @Test
    @DisplayName("the uncertainty flag must match the choice id")
    void uncertaintyFlagMismatchRejected() {
        TelemetryEvent event = choiceEvent("EP004", "S1", "SS_1", "GUNDUZ_ALP", false);

        assertThat(checker.check(event))
                .get()
                .extracting(TelemetrySanityChecker.Rejection::reason)
                .isEqualTo("UNCERTAINTY_FLAG_MISMATCH");
    }

    @Test
    @DisplayName("all seven uncertainty scenes map to their designed episodes")
    void uncertaintySceneEpisodeMapping() {
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(1)).isEqualTo(4);
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(2)).isEqualTo(12);
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(3)).isEqualTo(19);
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(4)).isEqualTo(26);
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(5)).isEqualTo(33);
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(6)).isEqualTo(41);
        assertThat(TelemetrySanityChecker.uncertaintySceneEpisode(7)).isEqualTo(48);
    }

    // ── fixtures ────────────────────────────────────────────────────────────

    private static TelemetryEvent woundEvent(String episodeId, String seasonId,
                                             float handIntegrity, float maxIntegrity,
                                             HandPhase phase, int deaths) {
        return new TelemetryEvent(
                UUID.randomUUID(), TelemetryEvent.Type.WOUND_STATE, null, "dev", UUID.randomUUID(),
                episodeId, seasonId, DifficultyTier.ALP, "1.0.0",
                Instant.now(), null,
                new TelemetryEvent.WoundState(handIntegrity, maxIntegrity, 41f, phase, 3, 2, deaths),
                null, Map.of());
    }

    private static TelemetryEvent choiceEvent(String episodeId, String seasonId,
                                              String choiceId, String optionId, boolean scene) {
        return new TelemetryEvent(
                UUID.randomUUID(), TelemetryEvent.Type.CHOICE_MADE, null, "dev", UUID.randomUUID(),
                episodeId, seasonId, DifficultyTier.ALP, "1.0.0",
                Instant.now(), null, null,
                new TelemetryEvent.Choice(choiceId, optionId, scene, 12_000),
                Map.of());
    }

    private static ErtugrulProperties testProperties() {
        return new ErtugrulProperties(
                new ErtugrulProperties.Security(
                        new ErtugrulProperties.Security.Jwt(
                                "https://test", "test", Duration.ofMinutes(30),
                                Duration.ofDays(30), "", ""),
                        "test-save-secret", "test-client-secret"),
                new ErtugrulProperties.S3("http://localhost:9000", "us-east-1", "k", "s",
                        true, "saves", "exports", Duration.ofMinutes(15)),
                new ErtugrulProperties.Save(8, 0, 8_388_608L, 10, Duration.ofMinutes(10)),
                new ErtugrulProperties.Telemetry("raw", "dlq", 200, Duration.ofDays(3)),
                new ErtugrulProperties.LiveOps(Duration.ofSeconds(60), "test-salt", ""),
                new ErtugrulProperties.RateLimit(true, 10, 20, 120, 300),
                new ErtugrulProperties.Journey(2000, Duration.ofDays(90), Duration.ofDays(7)));
    }
}
