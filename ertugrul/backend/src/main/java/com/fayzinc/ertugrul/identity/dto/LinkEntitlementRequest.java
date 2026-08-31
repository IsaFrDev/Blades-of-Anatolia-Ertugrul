package com.fayzinc.ertugrul.identity.dto;

import com.fayzinc.ertugrul.identity.Entitlement;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;

/**
 * Links a store account to the current player.
 *
 * <p>Klient platformadan bir martalik <i>ticket</i> oladi (Steam
 * {@code GetAuthSessionTicket}, PSN auth code, Xbox XSTS token) va uni shu
 * yerga yuboradi. Server uni platforma API'siga tekshirtiradi — klient
 * yuborgan {@code providerAccountId} ga <b>ishonilmaydi</b>, u faqat
 * ma'lumot uchun; haqiqiy identifikator ticket'dan chiqadi.
 *
 * @param provider          which store
 * @param sessionTicket     one-time platform ticket to verify server-side
 * @param claimedAccountId  client's view of its own store id; advisory only
 * @param productSku        which SKU is being claimed
 */
@Schema(name = "LinkEntitlementRequest", description = "Link a Steam/PSN/Xbox account")
public record LinkEntitlementRequest(

        @Schema(example = "STEAM", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotNull(message = "provider is required")
        Entitlement.Provider provider,

        @Schema(description = "One-time platform auth ticket, verified server-side",
                requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank(message = "sessionTicket is required")
        @Size(max = 4096)
        String sessionTicket,

        @Schema(example = "76561198000000000",
                description = "Advisory only — the authoritative id comes from ticket verification")
        @Size(max = 64)
        String claimedAccountId,

        @Schema(example = "DIRILIS_BASE", defaultValue = "DIRILIS_BASE")
        @Size(max = 64)
        String productSku
) {

    public LinkEntitlementRequest {
        productSku = (productSku == null || productSku.isBlank()) ? "DIRILIS_BASE" : productSku;
    }
}
