package com.fayzinc.ertugrul.telemetry;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.identity.PlayerRepository;
import com.fayzinc.ertugrul.integrity.TelemetrySanityChecker;
import com.fayzinc.ertugrul.stats.ChoiceAggregateService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

/**
 * Validates, stamps, and forwards incoming telemetry.
 *
 * <p>Kontrollerdan alohida, chunki bu yerda haqiqiy tranzaksiya bor: shubha
 * bali {@code player} qatorini o'zgartiradi va tanlovlar
 * {@code choice_vote_ledger} ga yoziladi. Tranzaksiyani kontrollerda ushlab
 * turish — HTTP qatlamiga tegishli bo'lmagan mas'uliyat va uni test qilishni
 * qiyinlashtiradi.
 *
 * <p>Rozilik (consent) shu yerda qo'llaniladi — quyi oqimda emas. GDPR
 * nuqtai nazaridan "keyin filtrlaymiz" yetarli emas: {@code OFF} bo'lsa
 * hodisa Kafka'ga <b>umuman</b> yozilmaydi.
 */
@Service
public class TelemetryIngestService {

    private static final Logger log = LoggerFactory.getLogger(TelemetryIngestService.class);

    private final TelemetryProducer producer;
    private final TelemetrySanityChecker sanityChecker;
    private final ChoiceAggregateService choiceAggregateService;
    private final PlayerRepository playerRepository;
    private final ErtugrulProperties props;

    public TelemetryIngestService(TelemetryProducer producer,
                                  TelemetrySanityChecker sanityChecker,
                                  ChoiceAggregateService choiceAggregateService,
                                  PlayerRepository playerRepository,
                                  ErtugrulProperties props) {
        this.producer = producer;
        this.sanityChecker = sanityChecker;
        this.choiceAggregateService = choiceAggregateService;
        this.playerRepository = playerRepository;
        this.props = props;
    }

    /**
     * Hodisalar paketini qabul qiladi, tekshiradi va Kafka'ga uzatadi.
     *
     * <p>Bosqichlar:
     * <ol>
     *   <li>Rozilik tekshiriladi — {@code OFF} bo'lsa hammasi tashlanadi;</li>
     *   <li>Har bir hodisa <b>server tomonidagi</b> {@code playerId} bilan
     *       muhrlanadi — klient yuborgan identifikatorga hech qachon
     *       ishonilmaydi;</li>
     *   <li>Sanity tekshiruvi: imkonsiz holatlar DLQ'ga ketadi va shubha
     *       balini oshiradi;</li>
     *   <li>Tanlovlar global taqsimotga qo'shiladi;</li>
     *   <li>Qolgani asinxron Kafka'ga yuboriladi.</li>
     * </ol>
     *
     * @param playerId the authenticated player
     * @param events   the batch to ingest
     * @return per-batch counts
     * @throws ErtugrulException when the batch exceeds the configured maximum
     */
    @Transactional
    public IngestResult ingest(UUID playerId, List<TelemetryEvent> events) {

        if (events.size() > props.telemetry().maxBatchSize()) {
            throw new ErtugrulException(ErtugrulException.Code.TELEMETRY_BATCH_TOO_LARGE,
                    "Batch of %d exceeds the limit of %d"
                            .formatted(events.size(), props.telemetry().maxBatchSize()));
        }

        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));

        if (!player.allowsTelemetry()) {
            // Settings -> Account & Cloud -> Telemetry: OFF. Accept and discard,
            // so the client stops retrying and the player's choice is honoured.
            return new IngestResult(0, 0, events.size(), "telemetry disabled by player");
        }

        boolean anonymous = player.getTelemetryConsent() == Player.TelemetryConsent.ANONYMOUS;
        UUID pseudonym = anonymous ? pseudonymFor(player) : null;

        List<TelemetryEvent> accepted = new ArrayList<>(events.size());
        int rejectedCount = 0;

        for (TelemetryEvent raw : events) {
            TelemetryEvent stamped = anonymous
                    ? raw.pseudonymised(pseudonym)
                    : raw.stamped(playerId);

            Optional<TelemetrySanityChecker.Rejection> rejection = sanityChecker.check(stamped);

            if (rejection.isPresent()) {
                rejectedCount++;
                TelemetrySanityChecker.Rejection reason = rejection.get();
                producer.publishRejected(stamped, reason.reason());

                if (reason.indicatesTampering()) {
                    // Physically impossible state. Raises suspicion, which only
                    // ever excludes the player from aggregate stats — never a ban.
                    player.raiseIntegritySuspicion(TelemetrySanityChecker.IMPOSSIBLE_EVENT_WEIGHT);
                    log.debug("Impossible telemetry from player={}: {}", playerId, reason.detail());
                }
                continue;
            }

            accepted.add(stamped);

            // Choice aggregation needs the real player id for vote dedupe. For an
            // ANONYMOUS player we deliberately skip it: a vote we cannot attribute
            // is a vote we cannot deduplicate, and a skewed split is worse than a
            // smaller one.
            if (stamped.type() == TelemetryEvent.Type.CHOICE_MADE
                    && stamped.choice() != null
                    && !anonymous) {
                choiceAggregateService.recordChoice(player, stamped);
            }
        }

        producer.publishBatch(accepted);

        return new IngestResult(accepted.size(), rejectedCount, 0, null);
    }

    /**
     * Anonim rejim uchun barqaror pseudonym.
     *
     * <p>Bir o'yinchi uchun doim bir xil bo'lishi kerak — aks holda funnel
     * ishlamaydi (bitta sessiyaning boshlanishi va tugashi bog'lanmaydi).
     * Ayni paytda u orqaga qaytarilmasligi kerak.
     *
     * <p>⚠️ TODO(FAYZ-231): hozircha akkaunt ID'sining o'zgarmas hosilasi.
     * Ishlab chiqarishga chiqishdan oldin bu davriy aylanadigan server tuzi
     * (salt) bilan almashtirilishi kerak — hozirgi shaklda kimdir
     * {@code playerId} ni bilsa, pseudonym'ni qayta hisoblab, anonim
     * hodisalarni shaxsga bog'lay oladi.
     */
    private static UUID pseudonymFor(Player player) {
        return UUID.nameUUIDFromBytes(("anon:" + player.getId()).getBytes());
    }

    /**
     * Per-batch outcome.
     *
     * @param accepted  events queued for Kafka
     * @param rejected  events that failed the sanity check
     * @param discarded events dropped because the player opted out
     * @param note      optional explanation
     */
    public record IngestResult(int accepted, int rejected, int discarded, String note) {
    }
}
