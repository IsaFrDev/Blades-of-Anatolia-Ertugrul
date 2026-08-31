package com.fayzinc.ertugrul.identity.dto;

import com.fayzinc.ertugrul.identity.PlayerDevice;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

/**
 * Silent, device-linked sign-in — the first network call the game ever makes.
 *
 * <p>Bu so'rovda parol ham, elektron pochta ham yo'q. O'yinchi "O'ynash"
 * tugmasini bosadi, klient o'zining barqaror {@code deviceId} sini yuboradi va
 * token oladi. Birinchi 60 soniyada ro'yxatdan o'tish oynasini ko'rsatmaslik —
 * bu ataylab qilingan mahsulot qarori.
 *
 * @param deviceId   stable, client-generated device identifier; also the vector-clock node key
 * @param platform   store/console the client is running on
 * @param appVersion client build, used for live-ops targeting and crash triage
 * @param locale     preferred language, e.g. {@code uz-Latn}
 */
@Schema(name = "DeviceLoginRequest", description = "Silent device-linked sign-in")
public record DeviceLoginRequest(

        @Schema(example = "d3f1c0de-7a11-4b2e-9c3d-8e5a1b2c3d4e", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank(message = "deviceId is required")
        @Size(min = 8, max = 64, message = "deviceId must be 8..64 characters")
        @Pattern(regexp = "^[A-Za-z0-9._:-]+$", message = "deviceId contains illegal characters")
        String deviceId,

        @Schema(example = "PC_STEAM", requiredMode = Schema.RequiredMode.REQUIRED)
        PlayerDevice.Platform platform,

        @Schema(example = "1.4.2+build.8871")
        @Size(max = 32)
        String appVersion,

        @Schema(example = "uz-Latn", defaultValue = "uz-Latn")
        @Size(max = 16)
        @Pattern(regexp = "^[a-zA-Z]{2}(-[A-Za-z]{2,8})?$", message = "locale must be a BCP-47 tag")
        String locale
) {

    /** Normalises optional fields so downstream code never branches on null. */
    public DeviceLoginRequest {
        platform = platform == null ? PlayerDevice.Platform.UNKNOWN : platform;
        locale = (locale == null || locale.isBlank()) ? "uz-Latn" : locale;
    }
}
