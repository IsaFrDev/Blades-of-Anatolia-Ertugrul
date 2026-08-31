package com.fayzinc.ertugrul.integrity;

import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.save.HandPhase;
import com.fayzinc.ertugrul.telemetry.TelemetryEvent;
import org.springframework.stereotype.Component;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

/**
 * Rejects telemetry that could not have come from a real playthrough.
 *
 * <p><b>Maqsad — anti-cheat emas, ma'lumot sifati.</b> O'yin single-player;
 * kimdir o'z save'ini o'zgartirsa, bu uning ishi. Yagona haqiqiy zarar —
 * imkonsiz raqamlar balans telemetriyasini buzishi va bizni <i>noto'g'ri
 * live-ops qaroriga</i> olib kelishi. Agar bitta buzilgan klient EP029 uchun
 * {@code hand_integrity = 0} yuboraversa, tizim epizodni "juda qattiq" deb
 * belgilaydi va biz uni haqiqiy sabab bo'lmasa ham osonlashtiramiz.
 *
 * <p>Tekshiruvlar o'yin dizaynidan to'g'ridan-to'g'ri kelib chiqadi
 * (05_MIH_SYSTEM.md), shuning uchun ular <i>fizik qonun</i> darajasida
 * qat'iy — "shubhali" emas, "imkonsiz".
 */
@Component
public class TelemetrySanityChecker {

    /**
     * Bir epizodda ishonarli o'lim soni chegarasi.
     *
     * <p>Bundan ko'pi — yo klient bug'i, yo o'yinchi ataylab o'lyapti. Ikkala
     * holatda ham bu ma'lumot balans o'lchoviga yaramaydi.
     */
    private static final int MAX_PLAUSIBLE_DEATHS_PER_EPISODE = 100;

    /** Opium is capped in design terms long before this; beyond it, the client is broken. */
    private static final int MAX_PLAUSIBLE_OPIUM_USES = 200;

    /** More flashbacks than this in one session is not a session, it is a loop. */
    private static final int MAX_PLAUSIBLE_FLASHBACKS_PER_SESSION = 200;

    /** Suspicion weight for an event that is physically impossible. */
    public static final int IMPOSSIBLE_EVENT_WEIGHT = 10;

    private final ErtugrulProperties props;

    public TelemetrySanityChecker(ErtugrulProperties props) {
        this.props = props;
    }

    /**
     * Hodisani tekshiradi.
     *
     * @param event the event to inspect
     * @return empty when the event is plausible, otherwise the reason to reject it
     */
    public Optional<Rejection> check(TelemetryEvent event) {

        List<String> problems = new ArrayList<>(2);

        checkTiming(event, problems);
        checkEpisodeConsistency(event, problems);

        if (event.wound() != null) {
            checkWoundState(event, problems);
        }
        if (event.choice() != null) {
            checkChoice(event, problems);
        }

        if (problems.isEmpty()) {
            return Optional.empty();
        }
        return Optional.of(new Rejection(problems.get(0), String.join("; ", problems)));
    }

    /**
     * Vaqt tekshiruvi.
     *
     * <p>Kelajakdagi hodisa — noto'g'ri sozlangan soat yoki qalbaki ma'lumot.
     * Juda eski hodisa — offline klient eskirgan navbatni bo'shatmoqda; u
     * <i>yolg'on emas</i>, lekin kunlik rollup'ga qo'shilsa o'sha kunning
     * statistikasini buzadi.
     */
    private void checkTiming(TelemetryEvent event, List<String> problems) {
        Instant now = Instant.now();

        if (event.occurredAt().isAfter(now.plus(Duration.ofMinutes(10)))) {
            problems.add("EVENT_IN_FUTURE");
        }
        if (event.occurredAt().isBefore(now.minus(props.telemetry().maxEventAge()))) {
            problems.add("EVENT_TOO_OLD");
        }
    }

    /** Episode and season must agree: 12 episodes per season, 4 seasons. */
    private void checkEpisodeConsistency(TelemetryEvent event, List<String> problems) {
        if (event.episodeId() == null || event.seasonId() == null) {
            return;
        }
        int episode = event.episodeNumber();
        if (episode == 0) {
            return;
        }

        // S1: EP001-012, S2: EP013-024, S3: EP025-036, S4: EP037-048.
        int expectedSeason = (episode - 1) / 12 + 1;
        int declaredSeason = event.seasonId().charAt(1) - '0';

        if (expectedSeason != declaredSeason) {
            problems.add("EPISODE_SEASON_MISMATCH");
        }
    }

    /**
     * Jarohat tizimining <b>buzib bo'lmaydigan qoidalari</b>.
     *
     * <p>Bular dizayndan kelib chiqadi va o'yin ichida hech qachon buzilmaydi:
     * <ul>
     *   <li>{@code handIntegrity} hech qachon {@code maxIntegrity} dan oshmaydi
     *       — shift ta'rifi bo'yicha shift;</li>
     *   <li>EP024 gacha faza {@code INTACT} bo'lishi shart — mix hali
     *       qoqilmagan;</li>
     *   <li>EP024 dan keyin {@code maxIntegrity} 55 dan oshmaydi, yagona
     *       istisno — EP043 protezi (+15, ya'ni 70 gacha);</li>
     *   <li>{@code INTACT} fazada {@code maxIntegrity} 100 bo'lishi kerak.</li>
     * </ul>
     */
    private void checkWoundState(TelemetryEvent event, List<String> problems) {
        TelemetryEvent.WoundState wound = event.wound();

        // The ceiling is a ceiling.
        if (wound.handIntegrity() > wound.maxIntegrity() + 0.01f) {
            problems.add("INTEGRITY_ABOVE_CEILING");
        }

        int episode = event.episodeNumber();
        if (episode > 0) {
            HandPhase expected = HandPhase.forEpisode(episode);

            // Before the nail scene the hand is whole. No exceptions.
            if (episode < HandPhase.NAIL_EPISODE && wound.phase().isWounded()) {
                problems.add("WOUNDED_BEFORE_NAIL_EPISODE");
            }

            // After the nail the ceiling never returns to 100. The only thing that
            // ever raises it is the EP043 prosthesis, which is why the bound is
            // read from the phase rather than hard-coded.
            if (episode >= HandPhase.NAIL_EPISODE
                    && wound.maxIntegrity() > expected.ceilingUpperBound() + 0.01f) {
                problems.add("CEILING_ABOVE_PHASE_MAXIMUM");
            }

            if (expected == HandPhase.INTACT && wound.maxIntegrity() < 99.99f) {
                problems.add("CEILING_LOWERED_BEFORE_NAIL");
            }
        }

        if (wound.deathsThisEpisode() > MAX_PLAUSIBLE_DEATHS_PER_EPISODE) {
            problems.add("IMPLAUSIBLE_DEATH_COUNT");
        }
        if (wound.opiumUsesTotal() > MAX_PLAUSIBLE_OPIUM_USES) {
            problems.add("IMPLAUSIBLE_OPIUM_COUNT");
        }
        if (wound.flashbacksThisSession() > MAX_PLAUSIBLE_FLASHBACKS_PER_SESSION) {
            problems.add("IMPLAUSIBLE_FLASHBACK_COUNT");
        }
    }

    /**
     * Tanlov tekshiruvi.
     *
     * <p>Shubha sahnalari faqat o'z epizodlarida bo'lishi mumkin — ular
     * hikoyaning aniq nuqtalariga bog'langan (02_HISTORY_LAYER.md §7).
     */
    private void checkChoice(TelemetryEvent event, List<String> problems) {
        TelemetryEvent.Choice choice = event.choice();

        if (choice.choiceId() == null || choice.optionId() == null) {
            problems.add("CHOICE_MISSING_IDS");
            return;
        }

        boolean looksLikeScene = choice.choiceId().matches(TelemetryEvent.UNCERTAINTY_SCENE_PATTERN);

        // The flag and the id must agree, or the stats split will be mislabelled.
        if (looksLikeScene != choice.uncertaintyScene()) {
            problems.add("UNCERTAINTY_FLAG_MISMATCH");
        }

        if (looksLikeScene && event.episodeNumber() > 0) {
            int sceneNumber = choice.choiceId().charAt(3) - '0';
            int expectedEpisode = uncertaintySceneEpisode(sceneNumber);
            if (expectedEpisode > 0 && expectedEpisode != event.episodeNumber()) {
                problems.add("UNCERTAINTY_SCENE_WRONG_EPISODE");
            }
        }
    }

    /**
     * Shubha sahnalarining epizodlari — 02_HISTORY_LAYER.md §7.
     *
     * <table>
     *   <caption>The seven uncertainty scenes</caption>
     *   <tr><td>SS_1</td><td>EP004</td><td>Who was your father?</td></tr>
     *   <tr><td>SS_2</td><td>EP012</td><td>Your tribe's name</td></tr>
     *   <tr><td>SS_3</td><td>EP019</td><td>Who poisoned Kayqubad?</td></tr>
     *   <tr><td>SS_4</td><td>EP026</td><td>Was Sogut given or taken?</td></tr>
     *   <tr><td>SS_5</td><td>EP033</td><td>How many soldiers at Kose Dag?</td></tr>
     *   <tr><td>SS_6</td><td>EP041</td><td>Whose is Karacahisar?</td></tr>
     *   <tr><td>SS_7</td><td>EP048</td><td>How do you write your lineage?</td></tr>
     * </table>
     *
     * @param sceneNumber 1..7
     * @return the episode the scene belongs to, or 0 when out of range
     */
    public static int uncertaintySceneEpisode(int sceneNumber) {
        return switch (sceneNumber) {
            case 1 -> 4;
            case 2 -> 12;
            case 3 -> 19;
            case 4 -> 26;
            case 5 -> 33;
            case 6 -> 41;
            case 7 -> 48;
            default -> 0;
        };
    }

    /**
     * Why an event was rejected.
     *
     * @param reason short machine-readable code, stored for spike detection
     * @param detail full list of problems, for debugging
     */
    public record Rejection(String reason, String detail) {

        /**
         * Whether this rejection reflects an impossible game state rather than a
         * merely stale or mistimed event.
         *
         * <p>Faqat imkonsiz holatlar shubha balini oshiradi. Eski hodisa —
         * offline o'ynagan halol o'yinchi, uni jazolash noto'g'ri bo'lardi.
         */
        public boolean indicatesTampering() {
            return !reason.startsWith("EVENT_");
        }
    }
}
