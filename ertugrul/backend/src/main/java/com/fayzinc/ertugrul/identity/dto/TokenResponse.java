package com.fayzinc.ertugrul.identity.dto;

import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.UUID;

/**
 * Issued token pair.
 *
 * <p>Access token qisqa umrli (30 daqiqa) — o'yinchi menyuda turganda klient
 * uni jimgina yangilaydi. Refresh token uzoq (30 kun), chunki o'yinchi o'yinni
 * bir necha hafta ochmasligi mumkin va qaytganda uni login oynasi kutib olmasligi
 * kerak.
 *
 * @param accessToken     RS256 JWT for the {@code Authorization: Bearer} header
 * @param refreshToken    opaque rotation token; stored hashed server-side
 * @param expiresAt       access token expiry
 * @param refreshExpiresAt refresh token expiry
 * @param playerId        the authenticated player
 * @param newAccount      true when this call created the account, so the client
 *                        can run first-run setup without a second round trip
 */
@Schema(name = "TokenResponse", description = "Issued access/refresh token pair")
public record TokenResponse(

        @Schema(example = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...")
        String accessToken,

        @Schema(example = "rt_9f8e7d6c5b4a...")
        String refreshToken,

        @Schema(example = "Bearer", defaultValue = "Bearer")
        String tokenType,

        Instant expiresAt,

        Instant refreshExpiresAt,

        UUID playerId,

        @Schema(description = "True when this request created a brand-new account")
        boolean newAccount
) {

    public static TokenResponse of(String accessToken, String refreshToken,
                                   Instant expiresAt, Instant refreshExpiresAt,
                                   UUID playerId, boolean newAccount) {
        return new TokenResponse(accessToken, refreshToken, "Bearer",
                expiresAt, refreshExpiresAt, playerId, newAccount);
    }
}
