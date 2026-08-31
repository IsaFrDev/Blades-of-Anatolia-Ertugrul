package com.fayzinc.ertugrul.stats.dto;

import io.swagger.v3.oas.annotations.media.Schema;

import java.util.List;

/**
 * "How did your choices compare" — the split for one choice.
 *
 * <p>Klient buni epizod tugagach ko'rsatadi. Matn tarjimasi klientda:
 * server faqat raqamlarni beradi, chunki ibora ("o'yinchilarning 62% i Titusni
 * kechirgan") lokalizatsiya qilinishi kerak va tilga qarab tuzilishi
 * o'zgaradi.
 *
 * @param choiceId         the choice
 * @param episodeId        where it happens
 * @param uncertaintyScene true for SS_1..SS_7, which get a different UI treatment
 * @param totalVotes       sample size; the client hides the split when it is too small
 * @param options          per-option counts and shares
 */
@Schema(name = "ChoiceSplit", description = "Aggregate distribution for one narrative choice")
public record ChoiceSplitResponse(

        @Schema(example = "SS_1")
        String choiceId,

        @Schema(example = "EP004")
        String episodeId,

        @Schema(description = "True for the seven uncertainty scenes, shown with scholarly attribution")
        boolean uncertaintyScene,

        long totalVotes,

        List<OptionShare> options
) {

    /**
     * Below this many votes the split is not shown at all.
     *
     * <p>Kichik namunada foiz aldamchi: 3 ta ovozda "67%" hech narsani
     * anglatmaydi, lekin o'yinchiga ishonchli ko'rinadi. Yangi chiqqan o'yinda
     * yoki kam o'ynaladigan tarmoqda bu tez-tez uchraydi.
     */
    public static final long MIN_VOTES_TO_DISPLAY = 100;

    /**
     * One option's share.
     *
     * @param optionId  the option
     * @param votes     raw count
     * @param percent   share of the total, 0..100, rounded to one decimal
     * @param playerPick true when this is the option the requesting player took
     */
    @Schema(name = "OptionShare")
    public record OptionShare(
            String optionId,
            long votes,
            @Schema(example = "62.4") double percent,
            boolean playerPick
    ) {
    }

    /** Whether the sample is large enough for the client to render the split. */
    public boolean displayable() {
        return totalVotes >= MIN_VOTES_TO_DISPLAY;
    }
}
