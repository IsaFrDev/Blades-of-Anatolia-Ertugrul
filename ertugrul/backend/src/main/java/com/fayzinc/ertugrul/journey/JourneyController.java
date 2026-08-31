package com.fayzinc.ertugrul.journey;

import com.fayzinc.ertugrul.identity.JwtService;
import com.fayzinc.ertugrul.journey.dto.ExportStatusResponse;
import com.fayzinc.ertugrul.journey.dto.JourneyEntryRequest;
import com.fayzinc.ertugrul.journey.dto.JourneyEntryResponse;
import com.fayzinc.ertugrul.journey.dto.PublicJourneyResponse;
import com.fayzinc.ertugrul.journey.dto.ShareLinkResponse;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.Size;
import org.springframework.http.CacheControl;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.time.Duration;
import java.util.List;
import java.util.UUID;

/**
 * Safar Daftari endpoints.
 *
 * <p>Diqqat: bu kontroller ichida <b>bitta ochiq (autentifikatsiyasiz)
 * endpoint</b> bor — {@code GET /shared/{token}}. U marketing uchun ataylab
 * ochiq: havolani ochgan odam o'yinni sotib olmagan bo'lishi mumkin.
 * Qolgan hamma narsa token talab qiladi.
 */
@RestController
@RequestMapping("/api/v1/journey")
@Validated
@Tag(name = "Journey", description = "Safar Daftari: sync, PDF export, public share links")
public class JourneyController {

    private final JourneyService journeyService;
    private final JourneyExportService exportService;
    private final JwtService jwtService;

    public JourneyController(JourneyService journeyService,
                             JourneyExportService exportService,
                             JwtService jwtService) {
        this.journeyService = journeyService;
        this.exportService = exportService;
        this.jwtService = jwtService;
    }

    @PostMapping("/entries")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Write a diary page",
            description = """
                    Idempotent on (playthroughId, sequenceNo): retrying after a dropped
                    connection returns the existing page rather than duplicating it.
                    Console networks drop often enough that this is a requirement, not a
                    nicety.

                    The text is composed by the client in the player's own voice; the
                    server never generates diary prose.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "201", description = "Page written"),
            @ApiResponse(responseCode = "429", description = "Per-player page limit reached")
    })
    public ResponseEntity<JourneyEntryResponse> writeEntry(
            Authentication authentication,
            @Valid @RequestBody JourneyEntryRequest request) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        JourneyEntryResponse saved = journeyService.writeEntry(playerId, request);
        return ResponseEntity.status(HttpStatus.CREATED).body(saved);
    }

    @GetMapping("/diaries")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "List the player's diaries",
            description = "NG+ starts a new diary rather than appending, so a player may have several.")
    public ResponseEntity<List<JourneyService.DiarySummary>> listDiaries(Authentication authentication) {
        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(journeyService.listDiaries(playerId));
    }

    @GetMapping("/diaries/{playthroughId}")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(summary = "Read one diary in full, in page order")
    public ResponseEntity<List<JourneyEntryResponse>> readDiary(
            Authentication authentication,
            @PathVariable UUID playthroughId) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(journeyService.readDiary(playerId, playthroughId));
    }

    @PutMapping("/entries/{entryId}/hidden")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Hide or unhide a page from shared links",
            description = "The page is kept; it simply stops appearing on public share pages.")
    public ResponseEntity<Void> setHidden(
            Authentication authentication,
            @PathVariable UUID entryId,
            @RequestParam boolean hidden) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        journeyService.setPageHidden(playerId, entryId, hidden);
        return ResponseEntity.noContent().build();
    }

    // ── Sharing ─────────────────────────────────────────────────────────────

    @PostMapping("/diaries/{playthroughId}/share")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Create a public share link",
            description = """
                    Returns an unguessable, expiring, revocable URL that resolves for
                    anyone — including people who do not own the game. That is the point:
                    a shared diary is the game's best advertisement.

                    The public page exposes diary text only. No player id, no save data,
                    nothing identifying.
                    """)
    @ApiResponse(responseCode = "201", description = "Link created")
    public ResponseEntity<ShareLinkResponse> createShareLink(
            Authentication authentication,
            @PathVariable UUID playthroughId,
            @RequestParam(required = false) @Size(max = 96) String title,
            @RequestParam(required = false) Integer fromSequenceNo,
            @RequestParam(required = false) Integer toSequenceNo) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        ShareLinkResponse link = journeyService.createShareLink(
                playerId, playthroughId, title, fromSequenceNo, toSequenceNo);

        return ResponseEntity.status(HttpStatus.CREATED).body(link);
    }

    @GetMapping("/shares")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(summary = "List the player's share links, live and revoked")
    public ResponseEntity<List<ShareLinkResponse>> listShareLinks(Authentication authentication) {
        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(journeyService.listShareLinks(playerId));
    }

    @DeleteMapping("/shares/{shareId}")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(summary = "Revoke a share link", description = "The URL stops resolving immediately.")
    public ResponseEntity<Void> revokeShareLink(
            Authentication authentication,
            @PathVariable UUID shareId) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        journeyService.revokeShareLink(playerId, shareId);
        return ResponseEntity.noContent().build();
    }

    /**
     * The one unauthenticated endpoint in the service.
     *
     * <p>Ochiq bo'lishi mahsulot qarori: daftar havolasi o'yin sotib olmagan
     * odamga ham ochilishi kerak. Shuning uchun u imkon qadar tor: faqat
     * o'qish, faqat token bo'yicha, faqat yashirilmagan sahifalar.
     */
    @GetMapping("/shared/{shareToken}")
    @Operation(
            summary = "Read a shared diary (public, no authentication)",
            description = """
                    Resolves a share token to its diary. Returns only what the player
                    chose to share: title, page text, and dates.

                    Deliberately unauthenticated — the audience for a shared diary is
                    people who do not have the game.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "The shared diary"),
            @ApiResponse(responseCode = "404", description = "Unknown token"),
            @ApiResponse(responseCode = "410", description = "Link expired or revoked")
    })
    public ResponseEntity<PublicJourneyResponse> readShared(
            @PathVariable @Size(max = 64) String shareToken) {

        PublicJourneyResponse shared = journeyService.readShared(shareToken);

        return ResponseEntity.ok()
                // Public and shared: a social-media crawler hitting this a thousand
                // times in a minute should be served by the CDN, not by us.
                .cacheControl(CacheControl.maxAge(Duration.ofMinutes(10)).cachePublic())
                .body(shared);
    }

    // ── Export ──────────────────────────────────────────────────────────────

    @PostMapping("/diaries/{playthroughId}/export")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Request a PDF export",
            description = """
                    Queues the job and returns immediately with status PENDING; poll the
                    status endpoint for the download URL.

                    Rendering is asynchronous because a full 48-episode diary is a real
                    document. Pages written after EP024 are typeset in an unsteady,
                    left-handed script — the wound made visible in the artefact the
                    player keeps.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "202", description = "Export queued"),
            @ApiResponse(responseCode = "404", description = "Diary has no pages")
    })
    public ResponseEntity<ExportStatusResponse> requestExport(
            Authentication authentication,
            @PathVariable UUID playthroughId,
            @RequestParam(defaultValue = "PDF") JourneyExport.Format format) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        ExportStatusResponse queued = exportService.requestExport(playerId, playthroughId, format);
        return ResponseEntity.accepted().body(queued);
    }

    @GetMapping("/exports/{exportId}")
    @SecurityRequirement(name = "bearerAuth")
    @Operation(
            summary = "Check an export's status",
            description = "Once READY the response carries a presigned download URL.")
    public ResponseEntity<ExportStatusResponse> exportStatus(
            Authentication authentication,
            @PathVariable UUID exportId) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(exportService.status(playerId, exportId));
    }
}
