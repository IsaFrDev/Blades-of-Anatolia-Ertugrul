package com.fayzinc.ertugrul.stats;

import jakarta.persistence.Column;
import jakarta.persistence.Embeddable;
import jakarta.persistence.EmbeddedId;
import jakarta.persistence.Entity;
import jakarta.persistence.Table;

import java.io.Serial;
import java.io.Serializable;
import java.time.Instant;
import java.util.Objects;

/**
 * How many players took a given option in a given choice.
 *
 * <p><b>Bu leaderboard emas.</b> O'yin single-player va raqobat yo'q. Bu
 * ma'lumot bitta narsa uchun: epizod tugagach o'yinchiga <i>oyna</i> tutish —
 * "o'yinchilarning 62% i Titusni kechirgan". Maqsad — o'yinchini o'z qarori
 * ustida yana bir bor o'ylashga majburlash, uni kimdir bilan solishtirish emas.
 *
 * <p>Eng qimmatlisi — 7 ta <b>Shubha sahnasi</b> ({@code SS_1}..{@code SS_7}),
 * u yerda o'yinchi bir-biriga zid tarixiy talqinlar orasidan tanlaydi. Ularning
 * statistikasi olimlar fikri bilan birga ko'rsatiladi va aynan shu narsa
 * o'yinni oddiy "tarixiy o'yin"dan tarix haqidagi <i>suhbatga</i> aylantiradi.
 */
@Entity
@Table(name = "choice_aggregate")
public class ChoiceAggregate {

    /**
     * Composite key: one row per (choice, option).
     *
     * @param choiceId the choice, e.g. {@code SS_1} or {@code EP012_TITUS_FATE}
     * @param optionId the option taken, e.g. {@code GUNDUZ_ALP} or {@code SPARE}
     */
    @Embeddable
    public static class ChoiceKey implements Serializable {

        @Serial
        private static final long serialVersionUID = 1L;

        @Column(name = "choice_id", nullable = false, length = 64)
        private String choiceId;

        @Column(name = "option_id", nullable = false, length = 64)
        private String optionId;

        protected ChoiceKey() {
            // JPA
        }

        public ChoiceKey(String choiceId, String optionId) {
            this.choiceId = choiceId;
            this.optionId = optionId;
        }

        public String getChoiceId() {
            return choiceId;
        }

        public String getOptionId() {
            return optionId;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof ChoiceKey other)) {
                return false;
            }
            return Objects.equals(choiceId, other.choiceId)
                    && Objects.equals(optionId, other.optionId);
        }

        @Override
        public int hashCode() {
            return Objects.hash(choiceId, optionId);
        }

        @Override
        public String toString() {
            return choiceId + "/" + optionId;
        }
    }

    @EmbeddedId
    private ChoiceKey key;

    @Column(name = "episode_id", nullable = false, length = 5)
    private String episodeId;

    /** True for {@code SS_1}..{@code SS_7} — the seven uncertainty scenes. */
    @Column(name = "uncertainty_scene", nullable = false)
    private boolean uncertaintyScene;

    /**
     * Times this option was taken.
     *
     * <p>Bitta o'yinchi bitta tanlov uchun bir marta sanaladi — NG+ da qayta
     * o'ynash global taqsimotni qiyshaytirmasligi kerak. Deduplikatsiya
     * {@code choice_vote_ledger} jadvalida.
     */
    @Column(name = "pick_count", nullable = false)
    private long pickCount;

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt = Instant.now();

    protected ChoiceAggregate() {
        // JPA
    }

    public ChoiceAggregate(ChoiceKey key, String episodeId, boolean uncertaintyScene) {
        this.key = key;
        this.episodeId = episodeId;
        this.uncertaintyScene = uncertaintyScene;
    }

    public ChoiceKey getKey() {
        return key;
    }

    public String getChoiceId() {
        return key == null ? null : key.getChoiceId();
    }

    public String getOptionId() {
        return key == null ? null : key.getOptionId();
    }

    public String getEpisodeId() {
        return episodeId;
    }

    public boolean isUncertaintyScene() {
        return uncertaintyScene;
    }

    public long getPickCount() {
        return pickCount;
    }

    public Instant getUpdatedAt() {
        return updatedAt;
    }
}
