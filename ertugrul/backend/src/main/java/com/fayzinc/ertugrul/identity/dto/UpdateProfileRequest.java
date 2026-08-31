package com.fayzinc.ertugrul.identity.dto;

import com.fayzinc.ertugrul.identity.Player;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;

/**
 * Partial profile update. Every field is optional — {@code null} means
 * "leave unchanged", which is what lets the settings screen PATCH just the
 * one toggle the player flipped.
 *
 * @param displayName      optional handle
 * @param locale           BCP-47 language tag
 * @param telemetryConsent FULL / ANONYMOUS / OFF
 * @param cloudSaveEnabled toggle cloud save sync
 * @param codexSyncEnabled toggle codex sync
 */
@Schema(name = "UpdateProfileRequest", description = "Partial profile update; null fields are untouched")
public record UpdateProfileRequest(

        @Size(max = 48)
        String displayName,

        @Size(max = 16)
        @Pattern(regexp = "^[a-zA-Z]{2}(-[A-Za-z]{2,8})?$", message = "locale must be a BCP-47 tag")
        String locale,

        Player.TelemetryConsent telemetryConsent,

        Boolean cloudSaveEnabled,

        Boolean codexSyncEnabled
) {
}
