package com.fayzinc.ertugrul.save;

/**
 * The four phases of the wound ("mix") system — 05_MIH_SYSTEM.md.
 *
 * <p>Bu o'yinning markaziy mexanikasi. EP024 da Ertug'rulning kaftiga mix
 * qoqiladi va u <b>hech qachon to'liq tuzalmaydi</b>: {@code MaxIntegrity}
 * shifti 100 dan 55 ga tushadi va faqat bitta narsa uni ko'taradi — EP043
 * dagi yunon suyak protezi (+15).
 *
 * <p>Server bu qiymatlarni hisoblamaydi; u faqat ularni saqlaydi, tekshiradi
 * va telemetriya orqali balansni kuzatadi. Ammo faza chegaralari serverda ham
 * bo'lishi shart — {@link com.fayzinc.ertugrul.integrity.TelemetrySanityChecker}
 * aynan shular yordamida imkonsiz holatlarni ajratadi.
 */
public enum HandPhase {

    /** EP001–EP023. The nail has not been driven yet; ceiling is still 100. */
    INTACT(1, 23, 100.0f),

    /** EP024–EP027. Fresh wound, ceiling drops to 55 and never returns. */
    FRESH(24, 27, 55.0f),

    /** EP028–EP042. Chronic. Mechanically the most punishing stretch. */
    CHRONIC(28, 42, 55.0f),

    /** EP043–EP048. Adapted; the EP043 prosthesis raises the ceiling by 15. */
    ADAPTED(43, 48, 70.0f);

    /** The ceiling MaxIntegrity drops to the moment the nail is driven. */
    public static final float NAIL_CEILING = 55.0f;

    /** The episode where the nail scene happens — the game's exact centre. */
    public static final int NAIL_EPISODE = 24;

    /** The only permanent improvement in the game: the Greek bone prosthesis. */
    public static final int PROSTHESIS_EPISODE = 43;
    public static final float PROSTHESIS_BONUS = 15.0f;

    private final int firstEpisode;
    private final int lastEpisode;
    private final float ceilingUpperBound;

    HandPhase(int firstEpisode, int lastEpisode, float ceilingUpperBound) {
        this.firstEpisode = firstEpisode;
        this.lastEpisode = lastEpisode;
        this.ceilingUpperBound = ceilingUpperBound;
    }

    /**
     * Epizod raqamiga qarab kutilayotgan fazani qaytaradi.
     *
     * @param episodeNumber 1..48
     * @return the phase the player should be in at that episode
     */
    public static HandPhase forEpisode(int episodeNumber) {
        for (HandPhase phase : values()) {
            if (episodeNumber >= phase.firstEpisode && episodeNumber <= phase.lastEpisode) {
                return phase;
            }
        }
        // Out of range: treat as INTACT rather than throwing. A malformed episode
        // id is the sanity checker's problem, not this lookup's.
        return INTACT;
    }

    /**
     * Shu fazada {@code maxIntegrity} qabul qilinadigan eng yuqori qiymat.
     *
     * <p>Bundan yuqorisi — imkonsiz: mix qoqilgandan keyin shift qaytmaydi.
     * Yagona istisno protez, u {@link #ADAPTED} fazasida hisobga olingan.
     */
    public float ceilingUpperBound() {
        return ceilingUpperBound;
    }

    public int firstEpisode() {
        return firstEpisode;
    }

    public int lastEpisode() {
        return lastEpisode;
    }

    /** True for every phase after the nail has been driven. */
    public boolean isWounded() {
        return this != INTACT;
    }
}
