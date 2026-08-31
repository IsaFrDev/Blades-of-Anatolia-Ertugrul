package com.fayzinc.ertugrul.config;

import com.fayzinc.ertugrul.telemetry.TelemetryEvent;
import org.apache.kafka.clients.admin.NewTopic;
import org.apache.kafka.clients.producer.ProducerConfig;
import org.apache.kafka.common.serialization.StringSerializer;
import org.springframework.boot.autoconfigure.kafka.KafkaProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.kafka.config.TopicBuilder;
import org.springframework.kafka.core.DefaultKafkaProducerFactory;
import org.springframework.kafka.core.KafkaTemplate;
import org.springframework.kafka.core.ProducerFactory;
import org.springframework.kafka.support.serializer.JsonSerializer;

import java.util.HashMap;
import java.util.Map;

/**
 * Kafka producer wiring for telemetry ingest.
 *
 * <p><b>Nega Kafka.</b> Telemetriya hajmi save trafigidan tartib kattaroq: har
 * sessiyada o'nlab {@code WOUND_STATE} va funnel hodisasi. Agar ular to'g'ridan
 * Postgres'ga yozilsa, telemetriya <i>save</i> xizmatini bo'g'adi — ya'ni
 * analitika o'yinchining progressini yo'qotishiga sabab bo'ladi. Kafka bu ikki
 * yukni bir-biridan ajratadi.
 *
 * <p><b>Durability trade-off:</b> {@code acks=1}. Telemetriya yo'qolsa —
 * statistika biroz noaniq bo'ladi; klientni kutdirsak — o'yin lag qiladi.
 * Bu yerda latency durability'dan muhimroq. Save blob'lari uchun teskarisi.
 */
@Configuration
public class KafkaConfig {

    private final ErtugrulProperties props;

    public KafkaConfig(ErtugrulProperties props) {
        this.props = props;
    }

    /**
     * Raw telemetry topic.
     *
     * <p>12 partitions: enough parallelism for the warehouse consumer group
     * without over-fragmenting. Partition key is the player id, which preserves
     * per-player ordering — required because {@code EPISODE_START} must not be
     * consumed after its matching {@code EPISODE_COMPLETE}.
     *
     * <p>7-day retention: the warehouse ETL runs hourly, so a week covers a long
     * weekend outage with room to spare.
     */
    @Bean
    public NewTopic telemetryRawTopic() {
        return TopicBuilder.name(props.telemetry().topic())
                .partitions(12)
                .replicas(1)                       // 3 in production
                .config("retention.ms", Long.toString(java.time.Duration.ofDays(7).toMillis()))
                .config("compression.type", "lz4")
                .config("cleanup.policy", "delete")
                .build();
    }

    /**
     * Events the sanity checker rejected.
     *
     * <p>Kept longer than the raw topic: a spike of one rejection reason is
     * almost always a client bug, and diagnosing it means looking back across
     * a release boundary.
     */
    @Bean
    public NewTopic telemetryRejectedTopic() {
        return TopicBuilder.name(props.telemetry().dlqTopic())
                .partitions(3)
                .replicas(1)
                .config("retention.ms", Long.toString(java.time.Duration.ofDays(30).toMillis()))
                .build();
    }

    @Bean
    public ProducerFactory<String, TelemetryEvent> telemetryProducerFactory(KafkaProperties kafkaProperties) {
        Map<String, Object> config = new HashMap<>(kafkaProperties.buildProducerProperties(null));

        config.put(ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG, StringSerializer.class);
        config.put(ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG, JsonSerializer.class);

        // Latency over durability — see class javadoc.
        config.put(ProducerConfig.ACKS_CONFIG, "1");
        config.put(ProducerConfig.COMPRESSION_TYPE_CONFIG, "lz4");

        // Batch window. 50ms of buffering turns a burst of per-event sends into
        // a handful of requests; the client already batches, so this is the
        // second, cheaper layer of batching.
        config.put(ProducerConfig.LINGER_MS_CONFIG, 50);
        config.put(ProducerConfig.BATCH_SIZE_CONFIG, 64 * 1024);

        // Bounded buffer: if Kafka is down, block briefly then fail fast rather
        // than growing the heap until the whole service dies.
        config.put(ProducerConfig.BUFFER_MEMORY_CONFIG, 64L * 1024 * 1024);
        config.put(ProducerConfig.MAX_BLOCK_MS_CONFIG, 2_000);
        config.put(ProducerConfig.DELIVERY_TIMEOUT_MS_CONFIG, 20_000);
        config.put(ProducerConfig.REQUEST_TIMEOUT_MS_CONFIG, 8_000);

        // Do not embed Java type headers: the warehouse consumer is not a JVM.
        config.put(JsonSerializer.ADD_TYPE_INFO_HEADERS, false);

        return new DefaultKafkaProducerFactory<>(config);
    }

    @Bean
    public KafkaTemplate<String, TelemetryEvent> telemetryKafkaTemplate(
            ProducerFactory<String, TelemetryEvent> telemetryProducerFactory) {
        KafkaTemplate<String, TelemetryEvent> template = new KafkaTemplate<>(telemetryProducerFactory);
        template.setDefaultTopic(props.telemetry().topic());
        // Micrometer picks these up as kafka.producer.* metrics.
        template.setObservationEnabled(true);
        return template;
    }
}
