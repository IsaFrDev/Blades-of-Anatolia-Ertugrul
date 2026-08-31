package com.fayzinc.ertugrul.telemetry;

import com.fayzinc.ertugrul.config.ErtugrulProperties;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.kafka.core.KafkaTemplate;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Component;

import java.util.List;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;

/**
 * Publishes telemetry to Kafka.
 *
 * <p><b>Ikki qat'iy qoida.</b>
 *
 * <p><b>1. Telemetriya hech qachon o'yinchini kutdirmaydi.</b> Ingest so'rovi
 * Kafka javobini kutmaydi — hodisalar navbatga qo'yiladi va so'rov darhol
 * {@code 202 Accepted} qaytaradi. O'yinchi analitika uchun bir millisekund
 * ham kutmasligi kerak.
 *
 * <p><b>2. Telemetriya hech qachon so'rovni yiqitmaydi.</b> Kafka o'chgan
 * bo'lsa ham klient muvaffaqiyat oladi. Yo'qolgan hodisa — statistikada
 * kichik noaniqlik; yiqilgan so'rov — o'yinchi ko'radigan xato. Bu almashuvda
 * tanlov aniq.
 *
 * <p>Partition kaliti — {@code playerId}: bu bir o'yinchining hodisalari
 * <b>tartibini saqlaydi</b>, ya'ni {@code EPISODE_COMPLETE} hech qachon
 * o'ziga mos {@code EPISODE_START} dan oldin iste'mol qilinmaydi.
 */
@Component
public class TelemetryProducer {

    private static final Logger log = LoggerFactory.getLogger(TelemetryProducer.class);

    private final KafkaTemplate<String, TelemetryEvent> kafkaTemplate;
    private final ErtugrulProperties props;

    private final Counter published;
    private final Counter failed;
    private final Counter rejected;

    public TelemetryProducer(KafkaTemplate<String, TelemetryEvent> telemetryKafkaTemplate,
                             ErtugrulProperties props,
                             MeterRegistry meterRegistry) {
        this.kafkaTemplate = telemetryKafkaTemplate;
        this.props = props;

        this.published = Counter.builder("ertugrul.telemetry.events")
                .tag("result", "published").register(meterRegistry);
        this.failed = Counter.builder("ertugrul.telemetry.events")
                .tag("result", "failed").register(meterRegistry);
        this.rejected = Counter.builder("ertugrul.telemetry.events")
                .tag("result", "rejected").register(meterRegistry);
    }

    /**
     * Hodisalar to'plamini asinxron yuboradi.
     *
     * <p>{@code @Async} + virtual thread'lar: har bir yuborish I/O kutishdan
     * iborat, shuning uchun platform thread band qilinmaydi.
     *
     * @param events already validated, stamped, and consent-filtered events
     */
    @Async
    public void publishBatch(List<TelemetryEvent> events) {
        for (TelemetryEvent event : events) {
            publishOne(event);
        }
    }

    /**
     * Bitta hodisani yuboradi.
     *
     * <p>Xatolik faqat log qilinadi va metrikaga yoziladi — u yuqoriga
     * tarqalmaydi. Kafka producer'i o'zining ichki buferi va qayta urinish
     * siyosatiga ega ({@code KafkaConfig}), shuning uchun bu yerda qo'shimcha
     * retry mantiqiga hojat yo'q.
     */
    private void publishOne(TelemetryEvent event) {
        String partitionKey = event.playerId() == null
                ? UUID.randomUUID().toString()   // consent OFF or anonymous: spread evenly
                : event.playerId().toString();

        try {
            CompletableFuture<?> future =
                    kafkaTemplate.send(props.telemetry().topic(), partitionKey, event);

            future.whenComplete((result, throwable) -> {
                if (throwable != null) {
                    failed.increment();
                    // DEBUG, not WARN: a broker blip would otherwise flood the log
                    // with one line per event. The counter is the real signal, and
                    // it is what the alert is built on.
                    log.debug("Telemetry publish failed for event={} type={}",
                            event.eventId(), event.type(), throwable);
                } else {
                    published.increment();
                }
            });

        } catch (Exception e) {
            // Buffer full or broker unreachable past max.block.ms. Swallowed on
            // purpose: see rule 2 in the class javadoc.
            failed.increment();
            log.debug("Telemetry publish rejected locally for event={}", event.eventId(), e);
        }
    }

    /**
     * Sanity tekshiruvidan o'tmagan hodisani DLQ topic'iga yuboradi.
     *
     * <p>Ular o'chirilmaydi: bitta rad etish sababining keskin o'sishi deyarli
     * har doim klient bug'i belgisidir, cheat emas — va uni tuzatish uchun
     * misollar kerak bo'ladi.
     *
     * @param event  the rejected event
     * @param reason machine-readable rejection code
     */
    @Async
    public void publishRejected(TelemetryEvent event, String reason) {
        rejected.increment();
        try {
            kafkaTemplate.send(props.telemetry().dlqTopic(), reason, event);
        } catch (Exception e) {
            log.debug("Could not publish rejected event={} to DLQ", event.eventId(), e);
        }
    }
}
