package com.fayzinc.ertugrul.identity;

import com.fayzinc.ertugrul.common.ApiError;
import com.fayzinc.ertugrul.identity.dto.DeviceLoginRequest;
import com.fayzinc.ertugrul.identity.dto.LinkEntitlementRequest;
import com.fayzinc.ertugrul.identity.dto.PlayerProfileResponse;
import com.fayzinc.ertugrul.identity.dto.RefreshRequest;
import com.fayzinc.ertugrul.identity.dto.TokenResponse;
import com.fayzinc.ertugrul.identity.dto.UpdateProfileRequest;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PatchMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.UUID;

/**
 * Authentication and profile endpoints.
 *
 * <p>Controller qatlami ataylab "yupqa": u faqat HTTP bilan ishlaydi — kirishni
 * validatsiya qiladi, JWT'dan {@code playerId} ni oladi, servisni chaqiradi va
 * status kodini tanlaydi. Biror biznes qarori bu yerda qabul qilinmaydi.
 */
@RestController
@RequestMapping("/api/v1/auth")
@Validated
@Tag(name = "Auth", description = "Device-linked accounts, JWT, store entitlements")
public class AuthController {

    private final AuthService authService;
    private final JwtService jwtService;

    public AuthController(AuthService authService, JwtService jwtService) {
        this.authService = authService;
        this.jwtService = jwtService;
    }

    @PostMapping("/device")
    @Operation(
            summary = "Silent device-linked sign-in",
            description = """
                    The first network call the game makes. A known device returns its
                    existing account; an unknown one creates a fresh anonymous account
                    with no player interaction at all.

                    Rate limited: see ertugrul.ratelimit.auth-per-minute.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Token pair issued"),
            @ApiResponse(responseCode = "403", description = "Account suspended",
                    content = @io.swagger.v3.oas.annotations.media.Content(
                            schema = @io.swagger.v3.oas.annotations.media.Schema(implementation = ApiError.class))),
            @ApiResponse(responseCode = "429", description = "Rate limited")
    })
    public ResponseEntity<TokenResponse> deviceLogin(@Valid @RequestBody DeviceLoginRequest request) {
        TokenResponse response = authService.deviceLogin(request);
        // 201 when the account is brand new, so client analytics can distinguish
        // installs from returning sessions without a second call.
        HttpStatus status = response.newAccount() ? HttpStatus.CREATED : HttpStatus.OK;
        return ResponseEntity.status(status).body(response);
    }

    @PostMapping("/refresh")
    @Operation(
            summary = "Rotate a refresh token",
            description = """
                    Returns a new token pair and invalidates the presented refresh token.

                    Presenting an already-used token is treated as theft: the entire
                    rotation family is revoked and REFRESH_TOKEN_REUSED is returned,
                    meaning the client must sign in again.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "New token pair issued"),
            @ApiResponse(responseCode = "401", description = "Token invalid, expired, or reused")
    })
    public ResponseEntity<TokenResponse> refresh(@Valid @RequestBody RefreshRequest request) {
        return ResponseEntity.ok(authService.refresh(request));
    }

    @GetMapping("/me")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(summary = "Current player profile and cloud preferences")
    public ResponseEntity<PlayerProfileResponse> me(Authentication authentication) {
        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(authService.getProfile(playerId));
    }

    @PatchMapping("/me")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Update profile settings",
            description = "Null fields are left unchanged, so the settings screen can PATCH a single toggle.")
    public ResponseEntity<PlayerProfileResponse> updateProfile(
            Authentication authentication,
            @Valid @RequestBody UpdateProfileRequest request) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(authService.updateProfile(playerId, request));
    }

    @PostMapping("/entitlements")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Link a Steam/PSN/Xbox account",
            description = """
                    Turns an anonymous device account into a recoverable one.

                    The client-supplied account id is advisory: the authoritative id
                    comes from server-side ticket verification. That verification is
                    currently a stub, so entitlements are stored as UNVERIFIED and
                    confirmed later by the revalidation job.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Entitlement linked"),
            @ApiResponse(responseCode = "409", description = "Store account already linked to another player")
    })
    public ResponseEntity<PlayerProfileResponse> linkEntitlement(
            Authentication authentication,
            @Valid @RequestBody LinkEntitlementRequest request) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(authService.linkEntitlement(playerId, request));
    }

    @PostMapping("/logout-all")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(summary = "Revoke every session on every device")
    public ResponseEntity<Void> logoutAll(Authentication authentication) {
        authService.logoutAll(jwtService.requirePlayerId(authentication));
        return ResponseEntity.noContent().build();
    }

    @DeleteMapping("/me")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Request GDPR erasure of all personal data",
            description = """
                    Settings -> Account & Cloud -> Delete my data.

                    Marks the account ERASURE_PENDING and revokes all sessions. The
                    actual scrub (PII, save blobs, journey entries) runs nightly, which
                    leaves a window to reverse an accidental request and satisfies the
                    platform certification requirement.
                    """)
    @ApiResponse(responseCode = "202", description = "Erasure scheduled")
    public ResponseEntity<Void> requestErasure(Authentication authentication) {
        authService.requestErasure(jwtService.requirePlayerId(authentication));
        return ResponseEntity.accepted().build();
    }
}
