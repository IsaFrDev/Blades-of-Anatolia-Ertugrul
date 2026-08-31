package com.fayzinc.ertugrul.identity.dto;

import com.fayzinc.ertugrul.identity.Entitlement;
import com.fayzinc.ertugrul.identity.Player;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.List;
import java.util.UUID;

/**
 * The player's own profile and cloud preferences.
 *
 * <p>Diqqat: {@code integrityScore} bu yerda <b>chiqarilmaydi</b>. Agar
 * o'yinchi o'z anti-cheat balini ko'rsa, u chegaradan pastda qolishni
 * o'rganadi — bu esa signalning butun ma'nosini yo'qotadi.
 *
 * @param playerId           account id
 * @param displayName        optional handle
 * @param locale             preferred language
 * @param telemetryConsent   FULL / ANONYMOUS / OFF
 * @param cloudSaveEnabled   whether saves sync
 * @param codexSyncEnabled   whether codex unlocks sync
 * @param entitlements       linked store accounts
 * @param recoverable        true once at least one valid entitlement exists
 * @param createdAt          account creation time
 */
@Schema(name = "PlayerProfileResponse", description = "Account profile and cloud preferences")
public record PlayerProfileResponse(

        UUID playerId,
        String displayName,
        String locale,
        Player.TelemetryConsent telemetryConsent,
        boolean cloudSaveEnabled,
        boolean codexSyncEnabled,
        List<LinkedStore> entitlements,

        @Schema(description = "True when the account can be recovered via a store login")
        boolean recoverable,

        Instant createdAt
) {

    /**
     * @param provider store name
     * @param sku      which product
     * @param status   VALID / REVOKED / REFUNDED / UNVERIFIED
     */
    @Schema(name = "LinkedStore")
    public record LinkedStore(
            Entitlement.Provider provider,
            String sku,
            Entitlement.Status status,
            Instant verifiedAt
    ) {
    }

    public static PlayerProfileResponse from(Player player, List<Entitlement> entitlements) {
        List<LinkedStore> linked = entitlements.stream()
                .map(e -> new LinkedStore(e.getProvider(), e.getProductSku(), e.getStatus(), e.getVerifiedAt()))
                .toList();

        boolean recoverable = entitlements.stream().anyMatch(Entitlement::isValid);

        return new PlayerProfileResponse(
                player.getId(),
                player.getDisplayName(),
                player.getLocale(),
                player.getTelemetryConsent(),
                player.isCloudSaveEnabled(),
                player.isCodexSyncEnabled(),
                linked,
                recoverable,
                player.getCreatedAt());
    }
}
