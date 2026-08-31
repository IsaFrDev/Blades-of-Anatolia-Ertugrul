package com.fayzinc.ertugrul.save.dto;

import com.fayzinc.ertugrul.save.DifficultyTier;
import com.fayzinc.ertugrul.save.Faction;
import com.fayzinc.ertugrul.save.HandPhase;
import io.swagger.v3.oas.annotations.media.Schema;
import jakarta.validation.constraints.DecimalMax;
import jakarta.validation.constraints.DecimalMin;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;
import jakarta.validation.constraints.PositiveOrZero;

import java.util.Map;

/**
 * The small, readable summary the client sends alongside an opaque save blob.
 *
 * <p><b>Nega bu kerak.</b> Blob serverga <i>tushunarsiz baytlar</i> — server uni
 * ochmaydi va ochmasligi kerak (format klientniki va tez-tez o'zgaradi). Lekin
 * ikki narsa uchun ochiq ma'lumot zarur:
 * <ol>
 *   <li><b>Yuklash ekrani</b> — 9 ta slotni ko'rsatish uchun S3'ga 9 marta
 *       borish mumkin emas;</li>
 *   <li><b>Integrity</b> — {@code hand_integrity > max_integrity} kabi
 *       <i>fizik jihatdan imkonsiz</i> holatlarni aniqlash.</li>
 * </ol>
 *
 * <p>Bu ma'lumot klientdan keladi, ya'ni <b>ishonchsiz</b>. U hech qachon
 * blob'ning haqiqiy mazmuni o'rniga ishlatilmaydi — faqat ko'rsatish va
 * shubha bali uchun.
 *
 * @param episodeId       EP001..EP048
 * @param seasonId        S1..S4
 * @param playtimeSeconds total play time in this playthrough
 * @param handIntegrity   current hand condition, 0..100
 * @param maxIntegrity    the ceiling; 100 pre-EP024, 55 after, 70 after the prosthesis
 * @param phase           INTACT / FRESH / CHRONIC / ADAPTED
 * @param sabr            patience, 0..100 — the counter-system to flashbacks
 * @param iman            inner steadiness, 0..100
 * @param factionReputation each faction in -100..100
 * @param worldFlags      narrative flags such as {@code Titus.Spared}, {@code Nail.Taken},
 *                        {@code father_lineage}
 * @param difficultyTier  LEGEND / ALP / FRONTIER / CHRONICLE
 * @param schemaVersion   blob layout version
 */
@Schema(name = "WorldStateSummary",
        description = "Readable summary accompanying the opaque save blob; untrusted, display + sanity only")
public record WorldStateSummary(

        @Schema(example = "EP024", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Pattern(regexp = "^EP0(0[1-9]|[1-3][0-9]|4[0-8])$", message = "episodeId must be EP001..EP048")
        String episodeId,

        @Schema(example = "S2", requiredMode = Schema.RequiredMode.REQUIRED)
        @NotBlank
        @Pattern(regexp = "^S[1-4]$", message = "seasonId must be S1..S4")
        String seasonId,

        @PositiveOrZero
        long playtimeSeconds,

        @Schema(example = "23.4", description = "Never displayed as a number in-game")
        @DecimalMin("0.0") @DecimalMax("100.0")
        float handIntegrity,

        @Schema(example = "55.0", description = "Drops to 55 at EP024 and never recovers")
        @DecimalMin("0.0") @DecimalMax("100.0")
        float maxIntegrity,

        @Schema(example = "CHRONIC")
        HandPhase phase,

        @Schema(example = "41.0")
        @DecimalMin("0.0") @DecimalMax("100.0")
        float sabr,

        @Schema(example = "62.0")
        @DecimalMin("0.0") @DecimalMax("100.0")
        float iman,

        @Schema(description = "Reputation per faction, each -100..100")
        Map<Faction, @Min(-100) @Max(100) Integer> factionReputation,

        @Schema(description = "Narrative flags, e.g. Titus.Spared=true, father_lineage=GUNDUZ_ALP",
                example = "{\"Titus.Spared\": \"true\", \"Nail.Taken\": \"true\", \"father_lineage\": \"GUNDUZ_ALP\"}")
        Map<String, String> worldFlags,

        DifficultyTier difficultyTier,

        @Min(1) @Max(1000)
        int schemaVersion
) {

    /** Cap on narrative flags, so a looping client cannot grow the row unbounded. */
    public static final int MAX_WORLD_FLAGS = 512;

    public WorldStateSummary {
        factionReputation = factionReputation == null ? Map.of() : Map.copyOf(factionReputation);
        worldFlags = worldFlags == null ? Map.of() : Map.copyOf(worldFlags);
        difficultyTier = difficultyTier == null ? DifficultyTier.ALP : difficultyTier;
        if (schemaVersion <= 0) {
            schemaVersion = 1;
        }
    }

    /** Episode number as an int, e.g. {@code EP024 -> 24}. */
    public int episodeNumber() {
        return Integer.parseInt(episodeId.substring(2));
    }

    /** The phase the player should be in, derived from the episode alone. */
    public HandPhase expectedPhase() {
        return HandPhase.forEpisode(episodeNumber());
    }
}
