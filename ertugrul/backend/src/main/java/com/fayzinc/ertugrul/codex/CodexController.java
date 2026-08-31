package com.fayzinc.ertugrul.codex;

import com.fayzinc.ertugrul.codex.dto.CodexSyncRequest;
import com.fayzinc.ertugrul.codex.dto.CodexSyncResponse;
import com.fayzinc.ertugrul.codex.dto.CodexUnlockDto;
import com.fayzinc.ertugrul.identity.JwtService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.Pattern;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.UUID;

/**
 * Codex progress endpoints — the history layer's server side.
 *
 * <p>Kodeks <i>mazmuni</i> bu yerda emas: matnlar, manbalar, tarjimalar CDN'da
 * JSON sifatida yotadi va live-ops ularni klient patch'isiz yangilaydi
 * (02_HISTORY_LAYER.md §9). Bu servis faqat <b>kim nimani ochgani</b> bilan
 * shug'ullanadi.
 */
@RestController
@RequestMapping("/api/v1/codex")
@Validated
@SecurityRequirement(name = "bearerAuth")
@Tag(name = "Codex", description = "~180 historical entries; cross-device union sync")
public class CodexController {

    private static final String CODEX_ID_PATTERN = "^CDX_[A-Z0-9_]{2,48}$";

    private final CodexSyncService codexSyncService;
    private final JwtService jwtService;

    public CodexController(CodexSyncService codexSyncService, JwtService jwtService) {
        this.codexSyncService = codexSyncService;
        this.jwtService = jwtService;
    }

    @PostMapping("/sync")
    @Operation(
            summary = "Bidirectional codex sync",
            description = """
                    Sends the client's unlocks and receives whatever the server has that
                    the client does not, in one round trip — console network calls are
                    expensive enough that splitting this in two is not worth it.

                    Codex progress is a grow-only set, so this merge is conflict-free by
                    construction and the call is fully idempotent: retrying after a
                    dropped connection is always safe.
                    """)
    @ApiResponse(responseCode = "200", description = "Merged state and collection statistics")
    public ResponseEntity<CodexSyncResponse> sync(
            Authentication authentication,
            @Valid @RequestBody CodexSyncRequest request) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(codexSyncService.sync(playerId, request));
    }

    @GetMapping
    @Operation(
            summary = "Full unlock snapshot",
            description = "Used on a fresh install or after the player asks to restore progress.")
    public ResponseEntity<List<CodexUnlockDto>> snapshot(Authentication authentication) {
        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(codexSyncService.fullSnapshot(playerId));
    }

    @PostMapping("/{codexId}/read")
    @Operation(
            summary = "Record that the player opened an entry",
            description = """
                    Fired every time the entry is opened, not just the first. The gap
                    between unlocking and reading is the primary measure of whether the
                    history layer is teaching anything.
                    """)
    @ApiResponse(responseCode = "204", description = "Recorded")
    public ResponseEntity<Void> markRead(
            Authentication authentication,
            @PathVariable @Pattern(regexp = CODEX_ID_PATTERN) String codexId) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        codexSyncService.markRead(playerId, codexId);
        return ResponseEntity.noContent().build();
    }

    @PutMapping("/{codexId}/bookmark")
    @Operation(summary = "Set or clear a bookmark on an unlocked entry")
    @ApiResponse(responseCode = "204", description = "Bookmark updated")
    public ResponseEntity<Void> setBookmark(
            Authentication authentication,
            @PathVariable @Pattern(regexp = CODEX_ID_PATTERN) String codexId,
            @RequestParam boolean bookmarked) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        codexSyncService.setBookmark(playerId, codexId, bookmarked);
        return ResponseEntity.noContent().build();
    }
}
