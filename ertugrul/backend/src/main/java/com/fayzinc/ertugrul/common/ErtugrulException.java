package com.fayzinc.ertugrul.common;

import org.springframework.http.HttpStatus;

import java.io.Serial;

/**
 * Domenga oid barcha kutilgan xatolar uchun yagona exception turi.
 *
 * <p>Har bir xato <b>machine-readable</b> {@link Code} olib yuradi. Sabab: o'yin
 * klienti (UE5) xato matnini o'qimaydi — u kodga qarab qaror qabul qiladi.
 * Masalan {@code SAVE_STALE} kelsa klient jimgina serverdan yangi save tortadi
 * va o'yinchini bezovta qilmaydi; {@code SAVE_CONFLICT} kelsa esa "qaysi
 * saqlashni tanlaysiz?" oynasini ko'rsatadi. Shuning uchun kod — API
 * kontraktining bir qismi, matn esa faqat log uchun.
 */
public class ErtugrulException extends RuntimeException {

    @Serial
    private static final long serialVersionUID = 1L;

    /**
     * Stable error codes. These are part of the public API contract — the game
     * client branches on them. Never rename one; add a new code instead.
     */
    public enum Code {

        // ── Identity ────────────────────────────────────────────────────────
        PLAYER_NOT_FOUND(HttpStatus.NOT_FOUND),
        INVALID_CREDENTIALS(HttpStatus.UNAUTHORIZED),
        REFRESH_TOKEN_INVALID(HttpStatus.UNAUTHORIZED),
        /** Rotated token reused → whole family revoked, client must re-login. */
        REFRESH_TOKEN_REUSED(HttpStatus.UNAUTHORIZED),
        ENTITLEMENT_INVALID(HttpStatus.FORBIDDEN),
        ENTITLEMENT_ALREADY_LINKED(HttpStatus.CONFLICT),
        ACCOUNT_SUSPENDED(HttpStatus.FORBIDDEN),

        // ── Save ────────────────────────────────────────────────────────────
        SAVE_SLOT_INVALID(HttpStatus.BAD_REQUEST),
        SAVE_NOT_FOUND(HttpStatus.NOT_FOUND),
        /** Incoming clock is an ancestor of the server head — client is behind. */
        SAVE_STALE(HttpStatus.CONFLICT),
        /** Vector clocks are concurrent — resolved by LWW, loser retained. */
        SAVE_CONFLICT(HttpStatus.CONFLICT),
        SAVE_TOO_LARGE(HttpStatus.PAYLOAD_TOO_LARGE),
        SAVE_CORRUPT(HttpStatus.UNPROCESSABLE_ENTITY),
        /** client_saved_at is implausibly far in the future. */
        SAVE_CLOCK_SKEW(HttpStatus.BAD_REQUEST),
        SAVE_SCHEMA_UNSUPPORTED(HttpStatus.UNPROCESSABLE_ENTITY),

        // ── Codex ───────────────────────────────────────────────────────────
        CODEX_ENTRY_UNKNOWN(HttpStatus.BAD_REQUEST),

        // ── Journey ─────────────────────────────────────────────────────────
        JOURNEY_ENTRY_NOT_FOUND(HttpStatus.NOT_FOUND),
        JOURNEY_QUOTA_EXCEEDED(HttpStatus.TOO_MANY_REQUESTS),
        SHARE_LINK_NOT_FOUND(HttpStatus.NOT_FOUND),
        SHARE_LINK_EXPIRED(HttpStatus.GONE),
        EXPORT_NOT_READY(HttpStatus.ACCEPTED),
        EXPORT_FAILED(HttpStatus.INTERNAL_SERVER_ERROR),

        // ── Telemetry ───────────────────────────────────────────────────────
        TELEMETRY_BATCH_TOO_LARGE(HttpStatus.PAYLOAD_TOO_LARGE),
        TELEMETRY_REJECTED(HttpStatus.UNPROCESSABLE_ENTITY),

        // ── Live-ops ────────────────────────────────────────────────────────
        CONFIG_NOT_FOUND(HttpStatus.NOT_FOUND),
        EPISODE_UNKNOWN(HttpStatus.BAD_REQUEST),

        // ── Cross-cutting ───────────────────────────────────────────────────
        RATE_LIMITED(HttpStatus.TOO_MANY_REQUESTS),
        VALIDATION_FAILED(HttpStatus.BAD_REQUEST),
        STORAGE_UNAVAILABLE(HttpStatus.SERVICE_UNAVAILABLE),
        INTERNAL(HttpStatus.INTERNAL_SERVER_ERROR);

        private final HttpStatus status;

        Code(HttpStatus status) {
            this.status = status;
        }

        public HttpStatus status() {
            return status;
        }
    }

    private final Code code;

    public ErtugrulException(Code code, String message) {
        super(message);
        this.code = code;
    }

    public ErtugrulException(Code code, String message, Throwable cause) {
        super(message, cause);
        this.code = code;
    }

    public Code code() {
        return code;
    }

    public HttpStatus status() {
        return code.status();
    }

    // ── Convenience factories for the hottest paths ─────────────────────────

    public static ErtugrulException playerNotFound(Object id) {
        return new ErtugrulException(Code.PLAYER_NOT_FOUND, "Player not found: " + id);
    }

    public static ErtugrulException saveNotFound(int slotIndex) {
        return new ErtugrulException(Code.SAVE_NOT_FOUND, "No save in slot " + slotIndex);
    }

    public static ErtugrulException invalidSlot(int slotIndex, int maxManual) {
        return new ErtugrulException(Code.SAVE_SLOT_INVALID,
                "Slot %d is out of range: 0 (autosave) or 1..%d".formatted(slotIndex, maxManual));
    }

    public static ErtugrulException rateLimited(String bucket) {
        return new ErtugrulException(Code.RATE_LIMITED, "Rate limit exceeded for " + bucket);
    }
}
