package com.fayzinc.ertugrul.codex.dto;

import com.fayzinc.ertugrul.codex.CodexCategory;
import com.fayzinc.ertugrul.codex.CodexEntryProgress;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.List;
import java.util.Map;

/**
 * The server's half of a codex sync.
 *
 * @param serverUnlocks entries the client did not have, or had staler copies of
 * @param acceptedCount how many of the client's unlocks were new to the server
 * @param totalUnlocked total entries this player has unlocked
 * @param byCategory    per-category counts for the completion bars
 * @param unreadByConfidence unlocked-but-unread counts, split by confidence
 * @param syncedAt      pass this back as {@code since} on the next sync
 */
@Schema(name = "CodexSyncResponse", description = "Merged codex state after a sync round")
public record CodexSyncResponse(

        List<CodexUnlockDto> serverUnlocks,

        int acceptedCount,

        @Schema(description = "Total unlocked; the base game ships 180 entries")
        long totalUnlocked,

        Map<CodexCategory, Long> byCategory,

        @Schema(description = """
                Unlocked but never opened, split by confidence. The gap between
                unlocking and reading is the key measure of whether the history
                layer is actually teaching anything.
                """)
        Map<CodexEntryProgress.Confidence, Long> unreadByConfidence,

        Instant syncedAt
) {
}
