package com.fayzinc.ertugrul.identity.dto;

import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Size;

/**
 * Exchanges a refresh token for a new pair.
 *
 * <p>Har chaqiruv token'ni <b>rotatsiya</b> qiladi: eskisi bir marta ishlatilgan
 * deb belgilanadi. Allaqachon ishlatilgan token qayta kelsa — butun oila bekor
 * qilinadi.
 *
 * @param refreshToken the opaque token from a previous issue
 * @param deviceId     must match the device the token was issued to
 */
@Schema(name = "RefreshRequest", description = "Refresh token rotation request")
public record RefreshRequest(

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank(message = "refreshToken is required")
        @Size(max = 512)
        String refreshToken,

        @Schema(requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank(message = "deviceId is required")
        @Size(min = 8, max = 64)
        String deviceId
) {
}
