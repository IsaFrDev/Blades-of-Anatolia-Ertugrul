package com.fayzinc.ertugrul.stats;

import com.fayzinc.ertugrul.identity.JwtService;
import com.fayzinc.ertugrul.stats.dto.ChoiceSplitResponse;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.Size;
import org.springframework.http.CacheControl;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.time.Duration;
import java.util.List;
import java.util.UUID;

/**
 * Aggregate choice statistics — <b>not</b> a leaderboard.
 *
 * <p>Bu endpoint'lar bitta savolga javob beradi: "boshqalar nima qildi?". U
 * o'yinchini kimdir bilan raqobatga qo'ymaydi va hech qanday reyting bermaydi.
 * Maqsad — epizod tugagach o'yinchiga oyna tutish: "o'yinchilarning 62% i
 * Titusni kechirgan". Bu — o'z qarori haqida yana bir bor o'ylash sababi.
 *
 * <p>Javoblar {@code Cache-Control} bilan beriladi: taqsimot sekundiga
 * o'zgarmaydi, epizod yakunida esa minglab klient bir vaqtda so'raydi.
 * Keshlash aynan shu portlashni yumshatadi.
 */
@RestController
@RequestMapping("/api/v1/stats/choices")
@Validated
@SecurityRequirement(name = "bearerAuth")
@Tag(name = "Stats", description = "Aggregate choice distribution — a mirror, not a scoreboard")
public class ChoiceStatsController {

    /**
     * Kesh muddati.
     *
     * <p>5 daqiqa — taqsimot amalda soatlab o'zgarmaydi (millionlab ovoz
     * ichida bitta yangi ovoz foizni siljitmaydi), lekin bu muddat o'yin
     * chiqqan birinchi kunlarda raqamlar tez to'planayotganda ham javobni
     * juda eskirtirmaydi.
     */
    private static final Duration CACHE_TTL = Duration.ofMinutes(5);

    private final ChoiceAggregateService choiceAggregateService;
    private final JwtService jwtService;

    public ChoiceStatsController(ChoiceAggregateService choiceAggregateService,
                                 JwtService jwtService) {
        this.choiceAggregateService = choiceAggregateService;
        this.jwtService = jwtService;
    }

    @GetMapping("/{choiceId}")
    @Operation(
            summary = "Distribution for one choice",
            description = """
                    Returns the share each option received, with the requesting player's
                    own pick flagged.

                    The client must check `totalVotes` before rendering: below the
                    display threshold a percentage looks authoritative while meaning
                    nothing, which is worse than showing no split at all.
                    """)
    @ApiResponse(responseCode = "200", description = "The split; empty options list when nobody has voted")
    public ResponseEntity<ChoiceSplitResponse> choiceSplit(
            Authentication authentication,
            @PathVariable @Size(max = 64) String choiceId) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        ChoiceSplitResponse split = choiceAggregateService.split(choiceId, playerId);

        return ResponseEntity.ok()
                // Private: the response embeds this player's own pick, so a shared
                // cache must never hand it to somebody else.
                .cacheControl(CacheControl.maxAge(CACHE_TTL).cachePrivate())
                .body(split);
    }

    @GetMapping("/uncertainty-scenes")
    @Operation(
            summary = "All seven uncertainty-scene splits",
            description = """
                    The end-of-game summary: the player's seven historical
                    interpretations shown beside everyone else's.

                    These are the choices where the sources genuinely conflict and
                    historians still disagree, so the client renders them with scholarly
                    attribution rather than as a simple majority.
                    """)
    public ResponseEntity<List<ChoiceSplitResponse>> uncertaintyScenes(Authentication authentication) {
        UUID playerId = jwtService.requirePlayerId(authentication);

        return ResponseEntity.ok()
                .cacheControl(CacheControl.maxAge(CACHE_TTL).cachePrivate())
                .body(choiceAggregateService.uncertaintySceneSplits(playerId));
    }

    @GetMapping("/episode/{episodeId}")
    @Operation(
            summary = "Every choice split recorded in one episode",
            description = "Shown on the episode epilogue screen, after the cliffhanger.")
    public ResponseEntity<List<ChoiceSplitResponse>> episodeSplits(
            Authentication authentication,
            @PathVariable
            @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
            String episodeId) {

        UUID playerId = jwtService.requirePlayerId(authentication);

        return ResponseEntity.ok()
                .cacheControl(CacheControl.maxAge(CACHE_TTL).cachePrivate())
                .body(choiceAggregateService.episodeSplits(episodeId, playerId));
    }
}
