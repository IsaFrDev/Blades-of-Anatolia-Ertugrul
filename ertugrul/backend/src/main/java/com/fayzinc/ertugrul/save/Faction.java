package com.fayzinc.ertugrul.save;

/**
 * The six factions the player holds a reputation with, each in {@code -100..100}.
 *
 * <p><b>Muvozanat qoidasi</b> (04_CORE_SYSTEMS.md §6): neytral qolib bo'lmaydi.
 * Bir fraksiyada obro' ko'tarilsa, uning dushmanida tushadi. Server bu
 * matritsani <i>hisoblamaydi</i> — u to'liq klientda, o'yin mantig'ida
 * qo'llaniladi. Bu yerdagi enum faqat save summary va telemetriya uchun
 * <i>lug'at</i>: server fraksiya obro'sini o'qiydi, lekin hech qachon
 * o'zgartirmaydi.
 */
public enum Faction {

    /** Konya sultanate. Wants loyalty, taxes, soldiers; gives land and protection. */
    SELJUK,

    /** Aleppo/Damascus. Wants trade and a quiet border; gives gold, learning, medicine. */
    AYYUBID,

    /** The Amanos castles. Wants control of the caravan road; gives iron and intelligence. */
    TEMPLAR,

    /** The Ilkhanate. Wants submission, tribute, a census; gives peace and the paiza. */
    MONGOL,

    /** Craftsman-sufi brotherhood. Wants justice and hospitality; gives economy and standing. */
    AHI,

    /** Nicaea, from season 4. Wants a stable frontier; gives medicine, trade, marriage. */
    BYZANTINE;

    /** Reputation bounds, inclusive. */
    public static final int MIN_REPUTATION = -100;
    public static final int MAX_REPUTATION = 100;

    /** Whether a reported reputation value is inside the legal range. */
    public static boolean isValidReputation(int value) {
        return value >= MIN_REPUTATION && value <= MAX_REPUTATION;
    }
}
