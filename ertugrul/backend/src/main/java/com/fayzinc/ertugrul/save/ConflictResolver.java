package com.fayzinc.ertugrul.save;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import java.time.Duration;
import java.time.Instant;

/**
 * Decides what happens when two devices write the same save slot.
 *
 * <p><b>Muammo.</b> O'yinchi PC'da EP023 gacha o'ynadi. PS5 esa internetsiz
 * qolib, EP021 dan davom etdi. Ikkalasi ham serverga yozmoqchi. Qaysi biri
 * "to'g'ri"?
 *
 * <p><b>Yechim — ikki bosqichli.</b>
 * <ol>
 *   <li><b>Vector clock hukm qiladi.</b> Agar kelayotgan soat serverdagidan
 *       aniq ustun bo'lsa — bu oddiy davomi, qabul qilinadi. Agar orqada bo'lsa —
 *       klient eskirgan, unga "avval tortib ol" deyiladi.</li>
 *   <li><b>Faqat ajralgan (CONCURRENT) holatda</b> last-write-wins ishlaydi:
 *       klient aytgan vaqtga qarab g'olib tanlanadi.</li>
 * </ol>
 *
 * <p><b>Eng muhim qoida:</b> LWW <b>hech qachon ma'lumot o'chirmaydi</b>.
 * Mag'lub tomonning blob'i saqlanib qoladi va o'yinchiga "boshqa qurilmada
 * boshqa saqlash bor edi — tiklaysizmi?" deb ko'rsatiladi. Single-player
 * o'yinda 40 daqiqalik progressni jimgina yo'qotish — eng yomon xato, va
 * uni tuzatib bo'lmaydi.
 *
 * <p><b>Nega LWW ni tanladik, avtomatik birlashtirish (merge) emas.</b> Save
 * blob'i — bog'langan grafik: kim tirik, qaysi qal'a kimda, qaysi kodeks
 * ochilgan, {@code hand_integrity} qancha. Ikki tarixni maydon bo'yicha
 * birlashtirsak, mantiqan <i>imkonsiz</i> dunyo hosil bo'ladi: Titus ham
 * kechirilgan, ham o'ldirilgan. Shuning uchun butun holat bir butun sifatida
 * tanlanadi.
 */
@Component
public class ConflictResolver {

    private static final Logger log = LoggerFactory.getLogger(ConflictResolver.class);

    /**
     * Teng vaqtlar uchun tolerantlik oynasi.
     *
     * <p>Ikki qurilmaning soati bir necha soniya farq qilishi normal. Shu oyna
     * ichida vaqtlar "teng" hisoblanadi va g'olib boshqa mezon bilan tanlanadi
     * (pastga qarang) — aks holda natija tasodifiy soat drift'iga bog'lanib
     * qolardi.
     */
    private static final Duration TIE_WINDOW = Duration.ofSeconds(5);

    /**
     * Kelayotgan yuklashni serverdagi bosh versiyaga nisbatan baholaydi.
     *
     * @param incoming    the clock the client is uploading
     * @param serverHead  the clock currently at the head of the slot, or empty for a new slot
     * @param incomingAt  the client's claimed save time
     * @param headAt      the head version's claimed save time
     * @param incomingProgress progress score of the incoming save, used as the tiebreak
     * @param headProgress     progress score of the current head
     * @return the decision the caller must apply
     */
    public Decision resolve(VectorClock incoming,
                            VectorClock serverHead,
                            Instant incomingAt,
                            Instant headAt,
                            ProgressScore incomingProgress,
                            ProgressScore headProgress) {

        if (serverHead == null || serverHead.isEmpty()) {
            return new Decision(Outcome.ACCEPT_INITIAL, incoming, "slot was empty");
        }

        VectorClock.Relation relation = incoming.relationTo(serverHead);

        return switch (relation) {

            case IDENTICAL -> new Decision(Outcome.ACCEPT_IDENTICAL, serverHead,
                    "identical clock — duplicate upload, treated as a no-op");

            // The client saw everything the server has, plus more. Normal case.
            case DESCENDS -> new Decision(Outcome.ACCEPT_FAST_FORWARD, incoming,
                    "incoming clock dominates the server head");

            // The client is behind: it saved from a state older than the server's.
            // Usually a second device that has not synced yet.
            case PRECEDES -> new Decision(Outcome.REJECT_STALE, serverHead,
                    "incoming clock is an ancestor of the server head");

            case CONCURRENT -> resolveConcurrent(
                    incoming, serverHead, incomingAt, headAt, incomingProgress, headProgress);
        };
    }

    /**
     * Ajralgan tarixlar uchun g'olibni tanlaydi.
     *
     * <p>Mezonlar tartibi ataylab shunday:
     * <ol>
     *   <li><b>Progress</b> — birinchi navbatda o'yinchining <i>ko'proq
     *       yutgan</i> tomoni. Epizod raqami, keyin o'ynalgan vaqt. Bu
     *       o'yinchining haqiqiy manfaatiga eng yaqin mezon.</li>
     *   <li><b>Vaqt</b> — progress teng bo'lsa, kechroq saqlangani.</li>
     *   <li><b>Determinizm</b> — hammasi teng bo'lsa, qurilma ID'si bo'yicha.
     *       Bu shart: bir xil kirish har doim bir xil natija berishi kerak,
     *       aks holda takroriy urinish boshqa javob qaytaradi.</li>
     * </ol>
     *
     * <p>Klassik LWW "eng oxirgi yozgani yutadi" deydi. Biz undan chetlashamiz,
     * chunki bu yerda soat ishonchsiz va, muhimi, <b>o'yinchi uchun "yangiroq"
     * emas, "ko'proq" muhim</b> — tasodifan ochilgan eski save yangi
     * timestamp bilan 10 soatlik progressni bosib ketmasligi kerak.
     */
    private Decision resolveConcurrent(VectorClock incoming,
                                       VectorClock serverHead,
                                       Instant incomingAt,
                                       Instant headAt,
                                       ProgressScore incomingProgress,
                                       ProgressScore headProgress) {

        VectorClock merged = incoming.merge(serverHead);

        // 1. Progress wins.
        int progressComparison = incomingProgress.compareTo(headProgress);
        if (progressComparison != 0) {
            boolean incomingWins = progressComparison > 0;
            log.info("Save conflict resolved by progress: incoming={} head={} -> {}",
                    incomingProgress, headProgress, incomingWins ? "INCOMING" : "HEAD");
            return new Decision(
                    incomingWins ? Outcome.CONFLICT_INCOMING_WINS : Outcome.CONFLICT_HEAD_WINS,
                    merged,
                    "concurrent clocks; resolved on progress");
        }

        // 2. Then the clock, outside the tie window.
        Duration gap = Duration.between(headAt, incomingAt);
        if (gap.abs().compareTo(TIE_WINDOW) > 0) {
            boolean incomingWins = gap.isPositive();
            log.info("Save conflict resolved by timestamp: gap={}s -> {}",
                    gap.toSeconds(), incomingWins ? "INCOMING" : "HEAD");
            return new Decision(
                    incomingWins ? Outcome.CONFLICT_INCOMING_WINS : Outcome.CONFLICT_HEAD_WINS,
                    merged,
                    "concurrent clocks; resolved on last-write-wins");
        }

        // 3. Deterministic fallback so a retry produces the same answer.
        boolean incomingWins = incoming.counters().toString()
                .compareTo(serverHead.counters().toString()) > 0;
        log.info("Save conflict fell through to deterministic tiebreak -> {}",
                incomingWins ? "INCOMING" : "HEAD");
        return new Decision(
                incomingWins ? Outcome.CONFLICT_INCOMING_WINS : Outcome.CONFLICT_HEAD_WINS,
                merged,
                "concurrent clocks; deterministic tiebreak");
    }

    /** What the caller must do with the upload. */
    public enum Outcome {
        /** Slot was empty; store as version 1. */
        ACCEPT_INITIAL,
        /** Same clock as the head; store nothing, return the existing head. */
        ACCEPT_IDENTICAL,
        /** Clean advance; store and move the head. */
        ACCEPT_FAST_FORWARD,
        /** Client is behind; reject and tell it to pull. */
        REJECT_STALE,
        /** Divergent, incoming wins; store as head, retain the old head as loser. */
        CONFLICT_INCOMING_WINS,
        /** Divergent, head wins; store the upload as a retained loser. */
        CONFLICT_HEAD_WINS;

        /** Whether the uploaded blob ends up as the slot's head. */
        public boolean becomesHead() {
            return this == ACCEPT_INITIAL
                    || this == ACCEPT_FAST_FORWARD
                    || this == CONFLICT_INCOMING_WINS;
        }

        /** Whether the client must be told a conflict happened. */
        public boolean isConflict() {
            return this == CONFLICT_INCOMING_WINS || this == CONFLICT_HEAD_WINS;
        }
    }

    /**
     * The resolver's verdict.
     *
     * @param outcome    what to do
     * @param mergedClock the clock to persist on the winning version
     * @param reason      human-readable explanation for logs and support
     */
    public record Decision(Outcome outcome, VectorClock mergedClock, String reason) {
    }

    /**
     * How far a save has progressed, used as the primary conflict tiebreak.
     *
     * <p>Epizod birinchi o'rinda: EP023 har doim EP021 dan ustun. Epizod teng
     * bo'lsa — o'ynalgan vaqt.
     *
     * @param episodeNumber   1..48
     * @param playtimeSeconds total time in this playthrough
     */
    public record ProgressScore(int episodeNumber, long playtimeSeconds)
            implements Comparable<ProgressScore> {

        public static ProgressScore of(int episodeNumber, long playtimeSeconds) {
            return new ProgressScore(episodeNumber, playtimeSeconds);
        }

        @Override
        public int compareTo(ProgressScore other) {
            int byEpisode = Integer.compare(episodeNumber, other.episodeNumber);
            if (byEpisode != 0) {
                return byEpisode;
            }
            return Long.compare(playtimeSeconds, other.playtimeSeconds);
        }

        @Override
        public String toString() {
            return "EP%03d/%ds".formatted(episodeNumber, playtimeSeconds);
        }
    }
}
