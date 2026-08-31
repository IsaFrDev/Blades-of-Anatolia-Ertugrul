package com.fayzinc.ertugrul.codex.dto;

import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

import java.time.Instant;
import java.util.List;

/**
 * A codex sync round.
 *
 * <p>Klient o'zida bor bo'lgan yozuvlarni yuboradi va oxirgi muvaffaqiyatli
 * sinxron vaqtini ({@code since}) ko'rsatadi. Server birlashmani hisoblaydi va
 * <i>klientda yo'q</i> yozuvlarni qaytaradi. Bu bitta so'rovda ikki
 * yo'nalishli sinxron — konsolda tarmoq chaqiruvlari qimmat, shuning uchun
 * ularni ikkiga bo'lish mantiqsiz.
 *
 * @param deviceId the syncing device
 * @param since    last successful sync; null means "send me everything"
 * @param unlocks  entries the client has that the server may not
 */
@Schema(name = "CodexSyncRequest", description = "Bidirectional codex sync in a single round trip")
public record CodexSyncRequest(

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Size(min = 8, max = 64)
        String deviceId,

        @Schema(description = "Last successful sync; null requests a full snapshot")
        Instant since,

        @Schema(description = "Entries unlocked on this device since the last sync")
        @Size(max = MAX_UNLOCKS_PER_SYNC, message = "too many unlocks in one sync round")
        List<@Valid CodexUnlockDto> unlocks
) {

    /**
     * Bir sinxronda yuborilishi mumkin bo'lgan maksimal yozuv.
     *
     * <p>O'yinda jami ~180 ta yozuv bor, shuning uchun 200 — to'liq
     * kollektsiyani bitta so'rovda yuborish uchun yetarli, lekin buzilgan
     * klient cheksiz ro'yxat yuborishining oldini oladi.
     */
    public static final int MAX_UNLOCKS_PER_SYNC = 200;

    public CodexSyncRequest {
        unlocks = unlocks == null ? List.of() : List.copyOf(unlocks);
    }
}
