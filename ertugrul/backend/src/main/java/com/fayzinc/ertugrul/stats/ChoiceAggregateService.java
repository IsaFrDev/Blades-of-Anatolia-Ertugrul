package com.fayzinc.ertugrul.stats;

import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.stats.dto.ChoiceSplitResponse;
import com.fayzinc.ertugrul.telemetry.TelemetryEvent;
import jakarta.persistence.EntityManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.math.BigDecimal;
import java.math.RoundingMode;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.stream.Collectors;

/**
 * Maintains and serves the aggregate choice distribution.
 *
 * <p><b>Uchta qoida bu servisning butun mantiqini belgilaydi.</b>
 *
 * <p><b>1. Har o'yinchi bitta ovoz.</b> NG+ da qayta o'ynash — o'yinning
 * dizayn maqsadi (7 ta Shubha sahnasi aynan shu uchun bor). Lekin agar har
 * o'tish sanalsa, ko'p o'ynagan o'yinchilar taqsimotni o'zlariga tortadi va
 * "62%" raqami o'yinchilar emas, <i>o'yin seanslari</i> haqida bo'lib qoladi.
 * Shuning uchun birinchi tanlov hisobga olinadi, keyingilari yo'q.
 *
 * <p><b>2. Shubhali akkauntlar sanalmaydi.</b> Buzilgan klientdan kelgan
 * tanlovlar global oynani ifloslantiradi.
 *
 * <p><b>3. Yozish atomik.</b> Hisoblagichni oshirish {@code ON CONFLICT DO
 * UPDATE} bilan qulfsiz bajariladi — JPA orqali "o'qi → oshir → yoz" qilinsa,
 * parallel yozuvlar bir-birini bosib ketardi.
 *
 * <p>O'qish tomoni keshlanadi, lekin bu yerda emas: taqsimot sekundiga
 * o'zgarmaydi, shuning uchun {@link ChoiceStatsController} javobga
 * {@code Cache-Control} qo'yadi va CDN uni o'zi ushlab qoladi. Servis ichida
 * yana bir kesh qatlami qo'shish — ikki joyda eskirish muammosi degani.
 */
@Service
public class ChoiceAggregateService {

    private static final Logger log = LoggerFactory.getLogger(ChoiceAggregateService.class);

    private final ChoiceAggregateRepository aggregateRepository;
    private final EntityManager entityManager;

    public ChoiceAggregateService(ChoiceAggregateRepository aggregateRepository,
                                  EntityManager entityManager) {
        this.aggregateRepository = aggregateRepository;
        this.entityManager = entityManager;
    }

    /**
     * Tanlovni global taqsimotga qo'shadi.
     *
     * <p>Telemetriya oqimidan {@code CHOICE_MADE} hodisasida chaqiriladi.
     * Ovoz faqat <b>birinchi marta</b> sanaladi: dedupe {@code choice_vote_ledger}
     * jadvalidagi {@code ON CONFLICT DO NOTHING} bilan atomik bajariladi, ya'ni
     * ikki parallel so'rov ham ikki ovoz yozib qo'ya olmaydi.
     *
     * @param player the voting player
     * @param event  a {@code CHOICE_MADE} event carrying a {@link TelemetryEvent.Choice}
     */
    @Transactional
    public void recordChoice(Player player, TelemetryEvent event) {
        TelemetryEvent.Choice choice = event.choice();
        if (choice == null || choice.choiceId() == null || choice.optionId() == null) {
            return;
        }

        // Rule 2: a suspicious account does not get to shape the global mirror.
        if (!player.countsTowardAggregateStats()) {
            log.debug("Skipping choice aggregation for player={} (integrity score {})",
                    player.getId(), player.getIntegrityScore());
            return;
        }

        // Rule 1: atomic first-vote-wins. A zero row count means this player has
        // already voted on this choice, so we stop before touching the counter.
        int inserted = entityManager.createNativeQuery("""
                        insert into choice_vote_ledger (player_id, choice_id, option_id, recorded_at)
                        values (:playerId, :choiceId, :optionId, now())
                        on conflict (player_id, choice_id) do nothing
                        """)
                .setParameter("playerId", player.getId())
                .setParameter("choiceId", choice.choiceId())
                .setParameter("optionId", choice.optionId())
                .executeUpdate();

        if (inserted == 0) {
            return;
        }

        incrementAggregate(
                choice.choiceId(),
                choice.optionId(),
                event.episodeId(),
                choice.uncertaintyScene());
    }

    /**
     * Hisoblagichni atomik oshiradi.
     *
     * <p>JPA orqali "o'qi → oshir → yoz" qilinsa, parallel yozuvlar bir-birini
     * bosib ketardi (lost update). {@code ON CONFLICT DO UPDATE SET x = x + 1}
     * esa bitta bo'linmas amal.
     */
    private void incrementAggregate(String choiceId, String optionId,
                                    String episodeId, boolean uncertaintyScene) {
        entityManager.createNativeQuery("""
                        insert into choice_aggregate
                            (choice_id, option_id, episode_id, uncertainty_scene, pick_count, updated_at)
                        values (:choiceId, :optionId, :episodeId, :scene, 1, now())
                        on conflict (choice_id, option_id)
                        do update set pick_count = choice_aggregate.pick_count + 1,
                                      updated_at = now()
                        """)
                .setParameter("choiceId", choiceId)
                .setParameter("optionId", optionId)
                .setParameter("episodeId", episodeId)
                .setParameter("scene", uncertaintyScene)
                .executeUpdate();
    }

    /**
     * Bitta tanlovning taqsimotini qaytaradi.
     *
     * <p>Agar so'ragan o'yinchi bu tanlovni qilgan bo'lsa, uning varianti
     * {@code playerPick} bilan belgilanadi — klient "siz buni tanladingiz"
     * deb ko'rsatishi uchun.
     *
     * @param choiceId        the choice to describe
     * @param requestingPlayer whose own pick should be highlighted; may be null
     * @return the split, or an empty split when nobody has voted yet
     */
    @Transactional(readOnly = true)
    public ChoiceSplitResponse split(String choiceId, UUID requestingPlayer) {
        List<ChoiceAggregate> rows = aggregateRepository.findByKeyChoiceId(choiceId);

        if (rows.isEmpty()) {
            return new ChoiceSplitResponse(choiceId, null, false, 0, List.of());
        }

        long total = rows.stream().mapToLong(ChoiceAggregate::getPickCount).sum();
        String playerOption = requestingPlayer == null ? null : lookupPlayerPick(requestingPlayer, choiceId);

        List<ChoiceSplitResponse.OptionShare> options = rows.stream()
                .map(row -> new ChoiceSplitResponse.OptionShare(
                        row.getOptionId(),
                        row.getPickCount(),
                        percentOf(row.getPickCount(), total),
                        row.getOptionId().equals(playerOption)))
                .sorted((a, b) -> Long.compare(b.votes(), a.votes()))
                .toList();

        ChoiceAggregate first = rows.get(0);
        return new ChoiceSplitResponse(
                choiceId, first.getEpisodeId(), first.isUncertaintyScene(), total, options);
    }

    /**
     * Barcha 7 ta Shubha sahnasining taqsimoti.
     *
     * <p>O'yin oxiridagi yakuniy ekran uchun: o'yinchi o'zining 7 ta tarixiy
     * talqin tanlovini butun hamjamiyat bilan yonma-yon ko'radi.
     *
     * @param requestingPlayer whose picks to highlight; may be null
     * @return one split per uncertainty scene, ordered SS_1..SS_7
     */
    @Transactional(readOnly = true)
    public List<ChoiceSplitResponse> uncertaintySceneSplits(UUID requestingPlayer) {
        Map<String, List<ChoiceAggregate>> byChoice =
                aggregateRepository.findByUncertaintySceneTrue().stream()
                        .collect(Collectors.groupingBy(ChoiceAggregate::getChoiceId));

        return byChoice.keySet().stream()
                .sorted()   // SS_1 .. SS_7
                .map(choiceId -> split(choiceId, requestingPlayer))
                .toList();
    }

    /** Splits for every choice recorded in one episode. */
    @Transactional(readOnly = true)
    public List<ChoiceSplitResponse> episodeSplits(String episodeId, UUID requestingPlayer) {
        return aggregateRepository.findByEpisodeId(episodeId).stream()
                .collect(Collectors.groupingBy(ChoiceAggregate::getChoiceId))
                .keySet().stream()
                .sorted()
                .map(choiceId -> split(choiceId, requestingPlayer))
                .toList();
    }

    // ── internals ───────────────────────────────────────────────────────────

    /** Which option this player took, or null if they have not made this choice. */
    private String lookupPlayerPick(UUID playerId, String choiceId) {
        List<?> result = entityManager.createNativeQuery("""
                        select option_id
                        from choice_vote_ledger
                        where player_id = :playerId
                          and choice_id = :choiceId
                        """)
                .setParameter("playerId", playerId)
                .setParameter("choiceId", choiceId)
                .getResultList();

        return result.isEmpty() ? null : String.valueOf(result.get(0));
    }

    /** Percentage to one decimal place; 0 when there are no votes at all. */
    private static double percentOf(long count, long total) {
        if (total <= 0) {
            return 0.0;
        }
        return BigDecimal.valueOf(count * 100.0 / total)
                .setScale(1, RoundingMode.HALF_UP)
                .doubleValue();
    }
}
