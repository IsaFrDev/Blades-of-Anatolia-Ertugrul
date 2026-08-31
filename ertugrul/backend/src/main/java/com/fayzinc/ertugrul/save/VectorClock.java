package com.fayzinc.ertugrul.save;

import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * A vector clock over device ids.
 *
 * <p><b>Nima uchun kerak.</b> O'yinchi PC'da EP023 gacha o'ynadi, PS5 esa
 * offline holda EP021 da qoldi. Faqat vaqt belgisiga qarab "qaysi yangi" deb
 * hukm chiqarish mumkin emas: soatlar noto'g'ri bo'lishi mumkin, va eng
 * muhimi — vaqt <i>ajralib ketgan</i> (divergent) ikki tarixni <i>ketma-ket</i>
 * ikki holatdan farqlay olmaydi. Vector clock aynan shu farqni ko'rsatadi.
 *
 * <p>Har bir qurilma o'z hisoblagichini oshiradi. Ikki soatni solishtirganda
 * to'rt holat bo'ladi — {@link Relation}.
 *
 * <p>Immutable: har bir amal yangi nusxa qaytaradi.
 *
 * @param counters device id → monotonically increasing counter
 */
public record VectorClock(Map<String, Long> counters) {

    /** How an incoming clock relates to the one already on the server. */
    public enum Relation {
        /** Byte-for-byte the same history — a duplicate upload. */
        IDENTICAL,
        /** Incoming strictly dominates: a clean fast-forward. */
        DESCENDS,
        /** Incoming is an ancestor: the client is behind and should pull. */
        PRECEDES,
        /** Neither dominates: two devices diverged. This is the real conflict. */
        CONCURRENT
    }

    /**
     * Vector clock'ni siqish chegarasi.
     *
     * <p>Har bir qurilma soatga tugun qo'shadi. O'yinchi qurilmalarni
     * almashtiraversa, soat cheksiz o'sadi va har save bilan tashiladi.
     * Shuning uchun eng eski tugunlar olib tashlanadi — bu xavfsiz, chunki
     * uzoq vaqt yozmagan qurilma qaytsa, u shunchaki "orqada" deb topiladi.
     */
    public static final int MAX_NODES = 16;

    public VectorClock {
        counters = counters == null
                ? Map.of()
                : Collections.unmodifiableMap(new TreeMap<>(counters));
    }

    public static VectorClock empty() {
        return new VectorClock(Map.of());
    }

    public static VectorClock of(Map<String, Long> counters) {
        return new VectorClock(counters);
    }

    /**
     * Shu qurilmaning hisoblagichini bittaga oshiradi.
     *
     * @param deviceId the writing device — the vector-clock node key
     * @return a new clock with {@code deviceId} incremented
     */
    public VectorClock increment(String deviceId) {
        Map<String, Long> next = new HashMap<>(counters);
        next.merge(deviceId, 1L, Long::sum);
        return new VectorClock(next);
    }

    /**
     * Ikki soatni birlashtiradi — har tugun bo'yicha maksimum.
     *
     * <p>Konflikt hal qilingandan keyin g'olib soat ikkalasining birlashmasini
     * olib yuradi, aks holda mag'lub tomonning tarixi "yo'qolgan" bo'lib
     * qoladi va keyingi yuklashda yana konflikt sifatida ko'rinadi.
     *
     * @param other clock to merge in
     * @return pointwise maximum of the two
     */
    public VectorClock merge(VectorClock other) {
        Map<String, Long> merged = new HashMap<>(counters);
        other.counters.forEach((node, value) -> merged.merge(node, value, Long::max));
        return new VectorClock(merged);
    }

    /**
     * Bu soat {@code other} ga nisbatan qanday joylashganini aniqlaydi.
     *
     * <p>Algoritm: har ikkala soatdagi barcha tugunlar bo'yicha yuriladi.
     * Agar biror tugunda {@code this} kattaroq bo'lsa — "oldinda" bayrog'i;
     * kichikroq bo'lsa — "orqada" bayrog'i. Ikkala bayroq ham ko'tarilsa,
     * tarixlar ajralgan: {@link Relation#CONCURRENT}.
     *
     * <p>Named {@code relationTo} rather than {@code compareTo} on purpose: this
     * is a partial order, and a method that looks like {@link Comparable} but
     * returns a fourth "incomparable" answer invites exactly the wrong reading.
     *
     * @param other the clock to compare against
     * @return the relation of {@code this} to {@code other}
     */
    public Relation relationTo(VectorClock other) {
        boolean anyGreater = false;
        boolean anyLess = false;

        Set<String> allNodes = new HashSet<>(counters.keySet());
        allNodes.addAll(other.counters.keySet());

        for (String node : allNodes) {
            // A node absent from a clock has implicitly never written: counter 0.
            long mine = counters.getOrDefault(node, 0L);
            long theirs = other.counters.getOrDefault(node, 0L);

            if (mine > theirs) {
                anyGreater = true;
            } else if (mine < theirs) {
                anyLess = true;
            }

            // Short-circuit: once both flags are set the answer cannot change.
            if (anyGreater && anyLess) {
                return Relation.CONCURRENT;
            }
        }

        if (!anyGreater && !anyLess) {
            return Relation.IDENTICAL;
        }
        return anyGreater ? Relation.DESCENDS : Relation.PRECEDES;
    }

    /**
     * Eng past hisoblagichli tugunlarni olib tashlab, soatni siqadi.
     *
     * <p>Faqat {@link #MAX_NODES} dan oshganda chaqiriladi. Eng past
     * hisoblagich — eng kam yozgan, ya'ni eng ehtimoli yuqori "tashlab
     * ketilgan" qurilma.
     *
     * @return a clock with at most {@link #MAX_NODES} entries
     */
    public VectorClock compact() {
        if (counters.size() <= MAX_NODES) {
            return this;
        }
        Map<String, Long> kept = counters.entrySet().stream()
                .sorted(Map.Entry.<String, Long>comparingByValue().reversed())
                .limit(MAX_NODES)
                .collect(HashMap::new, (m, e) -> m.put(e.getKey(), e.getValue()), HashMap::putAll);
        return new VectorClock(kept);
    }

    public long counterFor(String deviceId) {
        return counters.getOrDefault(deviceId, 0L);
    }

    public boolean isEmpty() {
        return counters.isEmpty();
    }
}
