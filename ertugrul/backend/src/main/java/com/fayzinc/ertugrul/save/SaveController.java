package com.fayzinc.ertugrul.save;

import com.fayzinc.ertugrul.identity.JwtService;
import com.fayzinc.ertugrul.save.dto.SaveDownloadResponse;
import com.fayzinc.ertugrul.save.dto.SaveSlotSummary;
import com.fayzinc.ertugrul.save.dto.SaveUploadRequest;
import com.fayzinc.ertugrul.save.dto.SaveUploadResponse;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
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
 * Cloud-save endpoints.
 *
 * <p>Slot raqamlash: <b>0 — avtosave</b> (dvijok o'zi yozadi), <b>1..8 —
 * qo'lda saqlash</b>. Bu chegara {@code ertugrul.save.max-manual-slots} da
 * sozlanadi, lekin o'yin dizayni 8 ta slotni ko'zda tutadi.
 */
@RestController
@RequestMapping("/api/v1/saves")
@Validated
@SecurityRequirement(name = "bearerAuth")
@Tag(name = "Saves", description = "Versioned cloud saves with vector-clock conflict resolution")
public class SaveController {

    private final SaveService saveService;
    private final JwtService jwtService;

    public SaveController(SaveService saveService, JwtService jwtService) {
        this.saveService = saveService;
        this.jwtService = jwtService;
    }

    @GetMapping
    @Operation(
            summary = "List every save slot",
            description = """
                    Returns all 9 slots (0 = autosave, 1..8 = manual), including empty
                    ones, so the Continue screen can render without client-side gap
                    filling. Served entirely from Postgres — no object-store reads.
                    """)
    public ResponseEntity<List<SaveSlotSummary>> listSlots(Authentication authentication) {
        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(saveService.listSlots(playerId));
    }

    @PutMapping("/{slotIndex}")
    @Operation(
            summary = "Upload a save to a slot",
            description = """
                    Idempotent per (slot, content hash): re-uploading identical bytes is
                    a no-op.

                    The response's `outcome` tells the client what happened:
                      * ACCEPT_* — proceed silently, the player notices nothing
                      * REJECT_STALE (409) — pull from the server first; still silent,
                        the player simply played elsewhere
                      * CONFLICT_* (409) — and ONLY here — show the player a choice.
                        The losing save is retained and restorable; nothing is deleted.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Stored, or resolved as a conflict"),
            @ApiResponse(responseCode = "400", description = "Bad slot, bad base64, or implausible client clock"),
            @ApiResponse(responseCode = "409", description = "Stale upload or divergent histories"),
            @ApiResponse(responseCode = "413", description = "Blob exceeds the size limit"),
            @ApiResponse(responseCode = "422", description = "Blob failed its hash check")
    })
    public ResponseEntity<SaveUploadResponse> upload(
            Authentication authentication,
            @PathVariable @Min(0) @Max(8) int slotIndex,
            @Valid @RequestBody SaveUploadRequest request) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(saveService.upload(playerId, slotIndex, request));
    }

    @GetMapping("/{slotIndex}")
    @Operation(
            summary = "Download the save in a slot",
            description = """
                    Small blobs come back inline as base64; larger ones as a presigned
                    object-store URL so the payload never passes through this service.

                    The stored server HMAC is verified before anything is returned: a
                    mismatch means store tampering or corruption, and the client gets an
                    error rather than a broken save.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Save returned"),
            @ApiResponse(responseCode = "404", description = "Slot is empty or missing"),
            @ApiResponse(responseCode = "422", description = "Stored save failed integrity verification")
    })
    public ResponseEntity<SaveDownloadResponse> download(
            Authentication authentication,
            @PathVariable @Min(0) @Max(8) int slotIndex,
            @RequestParam(defaultValue = "true") boolean inline) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(saveService.download(playerId, slotIndex, inline));
    }

    @PostMapping("/{slotIndex}/restore-conflict")
    @Operation(
            summary = "Restore the retained conflict copy",
            description = """
                    Swaps the retained losing save into the head position. The previous
                    head is itself retained, so the player can change their mind again.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "200", description = "Conflict copy restored"),
            @ApiResponse(responseCode = "404", description = "Nothing retained to restore")
    })
    public ResponseEntity<SaveSlotSummary> restoreConflict(
            Authentication authentication,
            @PathVariable @Min(0) @Max(8) int slotIndex) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.ok(saveService.restoreConflictCopy(playerId, slotIndex));
    }

    @PostMapping("/{slotIndex}/acknowledge-conflict")
    @Operation(
            summary = "Dismiss the conflict prompt",
            description = "Clears the slot's conflict flag once the player has chosen. The retained copy is kept.")
    public ResponseEntity<Void> acknowledgeConflict(
            Authentication authentication,
            @PathVariable @Min(0) @Max(8) int slotIndex) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        saveService.acknowledgeConflict(playerId, slotIndex);
        return ResponseEntity.noContent().build();
    }
}
