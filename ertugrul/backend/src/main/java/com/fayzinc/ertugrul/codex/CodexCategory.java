package com.fayzinc.ertugrul.codex;

/**
 * The eight codex categories — 02_HISTORY_LAYER.md §3.
 *
 * <p>Jami <b>180 ta yozuv</b>. Har kategoriyaning kutilgan soni shu yerda
 * yozilgan, chunki u ikki joyda kerak bo'ladi: kollektsiya foizini hisoblashda
 * ("142/180") va kontent CDN'dan yangi paket kelganda uni tekshirishda —
 * kutilmagan sondagi yozuv odatda buzilgan manifest belgisi.
 *
 * <p>Bu sonlar live-ops orqali o'sishi mumkin (yangi kodeks paketlari patchsiz
 * qo'shiladi), shuning uchun ular <i>bazaviy</i> qiymat: klient CDN
 * manifestidagi haqiqiy sonni ustun deb biladi.
 */
public enum CodexCategory {

    /** 34 entries: Ertugrul, Alaeddin Kayqubad, Baiju Noyan, Ibn Arabi, Sa'd al-Din Kopek... */
    PERSONS(34),

    /** 28 entries: Bagras castle, Konya, Aleppo, Sogut, Sultan Han, Kose Dag... */
    PLACES(28),

    /** 26 entries: composite bow, lamellar armour, the turko-mongol sabre, the nerge hunt... */
    WARFARE(26),

    /** 22 entries: uc bey, alp, beg, the Ahi brotherhood, futuwwa, atabegate, iqta. */
    SOCIETY(22),

    /** 18 entries: dirham, the caravanserai network, the silk road, Anatolian silver. */
    ECONOMY(18),

    /** 20 entries: Ibn Arabi's sufism, Mevlana, Shams, futuwwa ethics, Bektashism. */
    RELIGION(20),

    /** 22 entries: topak ev, kara cadir, yaylak/kislak transhumance, felt, food, medicine. */
    DAILY_LIFE(22),

    /** 10 entries: Yassicemen 1230, Kose Dag 1243, Nishapur 1221, Trapesac 1237. */
    EVENTS(10);

    private final int baselineEntryCount;

    CodexCategory(int baselineEntryCount) {
        this.baselineEntryCount = baselineEntryCount;
    }

    /** Entries shipped in the base game for this category. */
    public int baselineEntryCount() {
        return baselineEntryCount;
    }

    /** Total codex entries in the base game. Should be 180. */
    public static int baselineTotal() {
        int total = 0;
        for (CodexCategory category : values()) {
            total += category.baselineEntryCount;
        }
        return total;
    }
}
