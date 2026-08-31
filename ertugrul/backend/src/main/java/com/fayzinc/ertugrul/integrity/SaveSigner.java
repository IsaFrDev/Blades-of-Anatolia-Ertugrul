package com.fayzinc.ertugrul.integrity;

import com.fayzinc.ertugrul.config.ErtugrulProperties;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HexFormat;
import java.util.UUID;

/**
 * Signs and verifies save blobs.
 *
 * <p><b>Ikki xil imzo bor va ular butunlay boshqa maqsad uchun.</b>
 *
 * <p><b>1. Server HMAC</b> — kalit faqat serverda. U <i>(playerId, slot,
 * version, sha256)</i> ustidan hisoblanadi va blob'ning <b>kimligini</b>
 * bog'laydi. Nimadan himoya qiladi: object store'ga kirgan hujumchi bir
 * o'yinchining blob'ini boshqasiniki bilan almashtirishi yoki eski versiyani
 * yangisi o'rniga qo'yishi mumkin emas. Bu imzo <b>ishonchli</b>, chunki kalit
 * hech qachon klientga chiqmaydi.
 *
 * <p><b>2. Klient HMAC</b> — kalit o'yin binary'si ichida. Bu imzo
 * <b>ishonchsiz</b>: shipped binary'dan kalitni ajratib olish — bir necha
 * soatlik ish, va buni har bir modding hamjamiyati qiladi. Shuning uchun
 * klient imzosi mos kelmasa, save <b>rad etilmaydi</b> — u faqat shubha
 * balini oshiradi.
 *
 * <p><b>Nega rad etilmaydi.</b> O'yin single-player. O'yinchi o'z save'ini
 * o'zgartirsa, u faqat o'z tajribasini o'zgartiradi — bu uning haqqi va biz
 * unga to'sqinlik qilmaymiz. Yagona haqiqiy zarar — buzilgan ma'lumot global
 * statistikani ("62% Titusni kechirdi") ifloslantirishi. Shuning uchun jazo
 * bitta va aniq: bunday o'yinchining ovozi
 * {@link com.fayzinc.ertugrul.stats.ChoiceAggregateService} da sanalmaydi.
 * Anti-cheat sifatida sotilgan, aslida esa faqat halol o'yinchini bezovta
 * qiladigan tizimlar — bu o'yinning falsafasiga zid.
 */
@Component
public class SaveSigner {

    private static final Logger log = LoggerFactory.getLogger(SaveSigner.class);

    private static final String HMAC_ALGORITHM = "HmacSHA256";

    /** Suspicion weight added when a client-supplied HMAC does not verify. */
    public static final int CLIENT_HMAC_MISMATCH_WEIGHT = 5;

    private final SecretKeySpec serverKey;
    private final SecretKeySpec clientKey;

    public SaveSigner(ErtugrulProperties props) {
        this.serverKey = new SecretKeySpec(
                props.security().saveHmacSecret().getBytes(StandardCharsets.UTF_8), HMAC_ALGORITHM);
        this.clientKey = new SecretKeySpec(
                props.security().clientHmacSecret().getBytes(StandardCharsets.UTF_8), HMAC_ALGORITHM);
    }

    /**
     * Blob'ning <i>kimligi</i> ustidan server imzosini hisoblaydi.
     *
     * <p>Blob baytlarining o'zi emas, uning sha256 hash'i imzolanadi: bu 8 MiB
     * ni qayta o'qishdan qutqaradi, hash esa allaqachon hisoblangan.
     *
     * @param playerId  owning player
     * @param slotIndex slot the blob belongs to
     * @param version   version within the slot
     * @param sha256    content hash of the blob
     * @return 64-character lowercase hex HMAC
     */
    public String signBlobIdentity(UUID playerId, int slotIndex, long version, String sha256) {
        String payload = "%s|%d|%d|%s".formatted(playerId, slotIndex, version, sha256);
        return hmacHex(serverKey, payload);
    }

    /**
     * Yuklab olishda server imzosini tekshiradi.
     *
     * <p>Mos kelmasa — bu o'yinchining aybi emas: bu object store buzilgani
     * yoki blob almashtirilgani. Klientga buzuq ma'lumot berilmaydi.
     *
     * @return {@code true} when the stored signature matches
     */
    public boolean verifyBlobIdentity(UUID playerId, int slotIndex, long version,
                                      String sha256, String storedHmac) {
        String expected = signBlobIdentity(playerId, slotIndex, version, sha256);
        boolean valid = constantTimeEquals(expected, storedHmac);

        if (!valid) {
            log.error("Server HMAC mismatch for player={} slot={} version={} — possible store tampering",
                    playerId, slotIndex, version);
        }
        return valid;
    }

    /**
     * Klient imzosini tekshiradi — <b>yumshoq signal</b>.
     *
     * <p>Natija hech qachon so'rovni rad etmaydi. U faqat
     * {@link com.fayzinc.ertugrul.identity.Player#raiseIntegritySuspicion(int)}
     * ga uzatiladi.
     *
     * @param blob       the raw save bytes
     * @param clientHmac the signature the client sent, or null when it sent none
     * @return {@code null} when no signature was supplied, otherwise whether it verified
     */
    public Boolean verifyClientSignature(byte[] blob, String clientHmac) {
        if (clientHmac == null || clientHmac.isBlank()) {
            // Older clients predate signing. Absence is not evidence of anything.
            return null;
        }
        String expected = hmacHex(clientKey, blob);
        return constantTimeEquals(expected, clientHmac);
    }

    /**
     * Blob'ning sha256 hash'ini hisoblaydi va klient aytgani bilan solishtiradi.
     *
     * <p>Bu <i>xavfsizlik</i> tekshiruvi emas, <i>butunlik</i> tekshiruvi:
     * tarmoqda buzilgan yoki yarim yuklangan blob'ni saqlab qo'ymaslik uchun.
     *
     * @param blob           the received bytes
     * @param claimedSha256  what the client said the hash is
     * @return {@code true} when the content matches the claim
     */
    public boolean verifyContentHash(byte[] blob, String claimedSha256) {
        return constantTimeEquals(sha256Hex(blob), claimedSha256);
    }

    /** SHA-256 of raw bytes, lowercase hex. */
    public String sha256Hex(byte[] data) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return HexFormat.of().formatHex(digest.digest(data));
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
    }

    // ── internals ───────────────────────────────────────────────────────────

    private String hmacHex(SecretKeySpec key, String payload) {
        return hmacHex(key, payload.getBytes(StandardCharsets.UTF_8));
    }

    private String hmacHex(SecretKeySpec key, byte[] payload) {
        try {
            Mac mac = Mac.getInstance(HMAC_ALGORITHM);
            mac.init(key);
            return HexFormat.of().formatHex(mac.doFinal(payload));
        } catch (Exception e) {
            throw new IllegalStateException("HMAC computation failed", e);
        }
    }

    /**
     * Constant-time comparison.
     *
     * <p>Oddiy {@code equals} birinchi farqda to'xtaydi va shu orqali imzoni
     * bayt-bayt topish imkonini beradi (timing attack). Bu yerda xavf past,
     * lekin to'g'ri usul bir qator kodga arziydi.
     */
    private static boolean constantTimeEquals(String a, String b) {
        if (a == null || b == null) {
            return false;
        }
        return MessageDigest.isEqual(
                a.getBytes(StandardCharsets.UTF_8),
                b.getBytes(StandardCharsets.UTF_8));
    }
}
