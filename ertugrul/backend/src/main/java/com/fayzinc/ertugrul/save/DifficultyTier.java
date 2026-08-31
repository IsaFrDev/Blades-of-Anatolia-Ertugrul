package com.fayzinc.ertugrul.save;

/**
 * The four difficulty tiers — 04_CORE_SYSTEMS.md §9.1.
 *
 * <p>Nomlar o'yin ichidagi o'zbekcha atamalarga mos keladi: <i>Rivoyat</i>,
 * <i>Alp</i>, <i>Chegara</i>, <i>Xronika</i>. Server uchun ular faqat
 * telemetriya va live-ops balansini <b>ajratish o'lchovi</b>: EP029 dagi
 * o'lim soni faqat bir xil qiyinlikdagi o'yinchilar orasida solishtirilsa
 * ma'noga ega.
 */
public enum DifficultyTier {

    /** "Rivoyat" — for players who came for the story. Parry +80ms, wound decay x0.4. */
    LEGEND(80, 0.5f, 0.4f),

    /** "Alp" — the default, baseline everything. */
    ALP(0, 1.0f, 1.0f),

    /** "Chegara" — experienced players. Parry -20ms, damage x1.4. */
    FRONTIER(-20, 1.4f, 1.3f),

    /**
     * "Xronika" — unlocked in NG+. Permadeath for alps, strict dates, and only
     * documented historical elements are present.
     */
    CHRONICLE(-35, 1.8f, 1.6f);

    private final int parryWindowDeltaMs;
    private final float enemyDamageScale;
    private final float woundDecayScale;

    DifficultyTier(int parryWindowDeltaMs, float enemyDamageScale, float woundDecayScale) {
        this.parryWindowDeltaMs = parryWindowDeltaMs;
        this.enemyDamageScale = enemyDamageScale;
        this.woundDecayScale = woundDecayScale;
    }

    /**
     * Baseline values the client applies before any live-ops override.
     *
     * <p>Server bu qiymatlarni <i>qo'llamaydi</i> — u faqat ularni biladi, chunki
     * {@code EpisodeBalanceOverride} deltalari shu bazaviy qiymatlar ustiga
     * qo'shiladi va chegaradan chiqmasligi tekshiriladi.
     */
    public int parryWindowDeltaMs() {
        return parryWindowDeltaMs;
    }

    public float enemyDamageScale() {
        return enemyDamageScale;
    }

    public float woundDecayScale() {
        return woundDecayScale;
    }

    /** Lenient parse: an unknown tier from an old client falls back to the default. */
    public static DifficultyTier parseOrDefault(String raw) {
        if (raw == null || raw.isBlank()) {
            return ALP;
        }
        try {
            return valueOf(raw.trim().toUpperCase());
        } catch (IllegalArgumentException e) {
            return ALP;
        }
    }
}
