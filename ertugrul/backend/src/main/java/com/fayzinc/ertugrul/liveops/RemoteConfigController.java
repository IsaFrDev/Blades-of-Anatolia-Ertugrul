package com.fayzinc.ertugrul.liveops;

import com.fayzinc.ertugrul.identity.JwtService;
import com.fayzinc.ertugrul.save.DifficultyTier;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.DecimalMax;
import jakarta.validation.constraints.DecimalMin;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;
import org.springframework.http.HttpHeaders;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.UUID;

/**
 * Live-ops endpoints.
 *
 * <p>Klient tomoni ({@code /config}, {@code /balance/...}) — o'yin har
 * ishga tushganda va har epizod boshlanishida so'raydi. Admin tomoni
 * ({@code /admin/...}) — ichki live-ops paneli, {@code liveops:write}
 * huquqi bilan himoyalangan.
 *
 * <p><b>Muhim:</b> bu endpoint'larning hech biri o'yin uchun majburiy emas.
 * Server javob bermasa, klient o'zining shipped default'lari bilan davom
 * etadi — offline-first qoidasi.
 */
@RestController
@RequestMapping("/api/v1/liveops")
@Validated
@SecurityRequirement(name = "bearerAuth")
@Tag(name = "Live-ops", description = "Remote config, per-episode balance overrides, seasonal events")
public class RemoteConfigController {

    private final RemoteConfigService remoteConfigService;
    private final JwtService jwtService;

    public RemoteConfigController(RemoteConfigService remoteConfigService, JwtService jwtService) {
        this.remoteConfigService = remoteConfigService;
        this.jwtService = jwtService;
    }

    @GetMapping("/config")
    @Operation(
            summary = "Resolved live-ops document for this player",
            description = """
                    Returns remote config values, balance overrides, and running seasonal
                    events for the player's cohort.

                    Send the previous `revision` as `If-None-Match` to get a 304 when
                    nothing has changed — the client polls this on every launch, and the
                    document is usually identical.

                    Cohort assignment is a stable hash of the player id, so a player
                    never flips between A and B across sessions.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Document returned"),
            @ApiResponse(responseCode = "304", description = "Unchanged since the supplied revision")
    })
    public ResponseEntity<RemoteConfigService.LiveOpsDocument> config(
            Authentication authentication,
            @RequestParam(required = false) @Size(max = 32) String appVersion,
            @RequestParam(required = false) DifficultyTier difficultyTier,
            @RequestHeader(value = HttpHeaders.IF_NONE_MATCH, required = false) String ifNoneMatch) {

        UUID playerId = jwtService.requirePlayerId(authentication);

        RemoteConfigService.LiveOpsDocument document = remoteConfigService.resolve(
                playerId, appVersion,
                difficultyTier == null ? DifficultyTier.ALP : difficultyTier);

        String etag = "\"" + document.revision() + "\"";
        if (etag.equals(ifNoneMatch)) {
            return ResponseEntity.status(304).eTag(etag).build();
        }

        return ResponseEntity.ok().eTag(etag).body(document);
    }

    @GetMapping("/balance/{episodeId}")
    @Operation(
            summary = "Balance override for one episode",
            description = """
                    Requested just before an episode loads. Returns 204 when the episode
                    runs at baseline, which is the common case.

                    These values map 1:1 onto the client's own settings fields, so an
                    override is applied through the same already-tested code path as a
                    player-chosen accessibility option.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "An override applies to this player"),
            @ApiResponse(responseCode = "204", description = "Episode runs at baseline")
    })
    public ResponseEntity<RemoteConfigService.BalanceOverrideView> balance(
            Authentication authentication,
            @PathVariable
            @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
            String episodeId,
            @RequestParam(required = false) DifficultyTier difficultyTier) {

        UUID playerId = jwtService.requirePlayerId(authentication);

        RemoteConfigService.BalanceOverrideView override = remoteConfigService.balanceFor(
                playerId, episodeId,
                difficultyTier == null ? DifficultyTier.ALP : difficultyTier);

        return override == null
                ? ResponseEntity.noContent().build()
                : ResponseEntity.ok(override);
    }

    // ── Admin surface (liveops:write) ───────────────────────────────────────

    @PutMapping("/admin/balance/{episodeId}")
    @Operation(
            summary = "Create or update an episode balance override",
            description = """
                    The live-ops dashboard's main lever. Typically used after
                    wound_balance_daily flags an episode as too punishing: lower
                    woundDecayScale, roll out to 10%, watch, then widen.

                    Every value is bounded both here and by a CHECK constraint, so a
                    dashboard typo cannot make an episode unplayable.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Override stored and caches evicted"),
            @ApiResponse(responseCode = "400", description = "A value is outside its designed range"),
            @ApiResponse(responseCode = "403", description = "Missing the liveops:write scope")
    })
    public ResponseEntity<RemoteConfigService.BalanceOverrideView> upsertBalance(
            Authentication authentication,
            @PathVariable
            @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
            String episodeId,
            @Valid @RequestBody BalanceUpdateRequest request) {

        RemoteConfigService.BalanceOverrideView saved = remoteConfigService.upsertBalance(
                episodeId,
                request.variantKey(),
                request.parryWindowMsDelta(),
                request.enemyDamageScale(),
                request.woundDecayScale(),
                request.enemyCountScale(),
                request.rolloutPercent(),
                authentication.getName());

        return ResponseEntity.ok(saved);
    }

    /**
     * A balance change from the live-ops dashboard.
     *
     * <p>Chegaralar dizayndan kelib chiqadi (04_CORE_SYSTEMS.md §1.3,
     * 05_MIH_SYSTEM.md §2) va bu yerda ham, SQL'da ham tekshiriladi.
     *
     * @param variantKey         experiment variant, or {@code default}
     * @param parryWindowMsDelta added to the base parry window, -60..120 ms
     * @param enemyDamageScale   incoming damage multiplier, 0.25..2.5
     * @param woundDecayScale    HandIntegrity decay multiplier, 0.2..2.0
     * @param enemyCountScale    encounter size multiplier, 0.5..2.0
     * @param rolloutPercent     cohort share receiving this, 0..100
     * @param notes              why the change was made; kept for the audit trail
     */
    public record BalanceUpdateRequest(

            @NotBlank
            @Size(max = 32)
            String variantKey,

            @Min(-60) @Max(120)
            int parryWindowMsDelta,

            @DecimalMin("0.25") @DecimalMax("2.5")
            float enemyDamageScale,

            @DecimalMin("0.2") @DecimalMax("2.0")
            float woundDecayScale,

            @DecimalMin("0.5") @DecimalMax("2.0")
            float enemyCountScale,

            @Min(0) @Max(100)
            int rolloutPercent,

            @Size(max = 500)
            String notes
    ) {
    }
}
