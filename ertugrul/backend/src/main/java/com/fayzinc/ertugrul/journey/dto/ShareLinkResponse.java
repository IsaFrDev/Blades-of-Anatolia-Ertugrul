package com.fayzinc.ertugrul.journey.dto;

import com.fayzinc.ertugrul.journey.JourneyShare;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.UUID;

/**
 * A created or listed share link.
 *
 * @param id         internal id, for revoking it later
 * @param shareToken the token; the client builds the URL from it
 * @param shareUrl   the ready-to-share URL
 * @param publicTitle what viewers will see as the title
 * @param viewCount  how many times it has been opened
 * @param expiresAt  when the link stops working
 * @param revoked    whether the player has already revoked it
 */
@Schema(name = "ShareLink", description = "A public link to a diary")
public record ShareLinkResponse(

        UUID id,
        String shareToken,

        @Schema(example = "https://dirilis-game.com/daftar/8f2a1c9e4b7d6350")
        String shareUrl,

        String publicTitle,
        long viewCount,
        Instant expiresAt,
        boolean revoked
) {

    public static ShareLinkResponse from(JourneyShare share, String baseUrl) {
        return new ShareLinkResponse(
                share.getId(),
                share.getShareToken(),
                baseUrl + "/daftar/" + share.getShareToken(),
                share.getPublicTitle(),
                share.getViewCount(),
                share.getExpiresAt(),
                share.getRevokedAt() != null);
    }
}
