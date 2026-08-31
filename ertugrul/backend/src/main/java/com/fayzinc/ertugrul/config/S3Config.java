package com.fayzinc.ertugrul.config;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import software.amazon.awssdk.auth.credentials.AwsBasicCredentials;
import software.amazon.awssdk.auth.credentials.StaticCredentialsProvider;
import software.amazon.awssdk.core.client.config.ClientOverrideConfiguration;
import software.amazon.awssdk.core.retry.RetryPolicy;
import software.amazon.awssdk.core.retry.backoff.EqualJitterBackoffStrategy;
import software.amazon.awssdk.core.retry.conditions.RetryCondition;
import software.amazon.awssdk.http.apache.ApacheHttpClient;
import software.amazon.awssdk.regions.Region;
import software.amazon.awssdk.services.s3.S3Client;
import software.amazon.awssdk.services.s3.S3Configuration;
import software.amazon.awssdk.services.s3.presigner.S3Presigner;

import java.net.URI;
import java.time.Duration;

/**
 * S3 / MinIO client wiring for save blobs and Safar Daftari exports.
 *
 * <p><b>Nega ikki bucket.</b> {@code ertugrul-saves} — xususiy va versiyalangan;
 * {@code ertugrul-exports} — ommaviy o'qish uchun, chunki Safar Daftari PDF
 * havolasi o'yin sotib olmagan odamga ham ochilishi kerak. Ularni aralashtirish
 * — save blob'lari ommaviy bo'lib qolish xavfi degani, shuning uchun ular
 * <b>hech qachon bitta bucket'da bo'lmaydi</b>.
 */
@Configuration
public class S3Config {

    private static final Logger log = LoggerFactory.getLogger(S3Config.class);

    private final ErtugrulProperties props;

    public S3Config(ErtugrulProperties props) {
        this.props = props;
    }

    @Bean
    public S3Client s3Client() {
        ErtugrulProperties.S3 cfg = props.s3();

        S3Configuration serviceConfig = S3Configuration.builder()
                // MinIO addresses buckets as /bucket/key, not bucket.host/key.
                .pathStyleAccessEnabled(cfg.pathStyleAccess())
                .chunkedEncodingEnabled(false)
                .build();

        log.info("S3 client -> endpoint={} region={} pathStyle={} saveBucket={} exportBucket={}",
                cfg.endpoint(), cfg.region(), cfg.pathStyleAccess(), cfg.saveBucket(), cfg.exportBucket());

        return S3Client.builder()
                .endpointOverride(URI.create(cfg.endpoint()))
                .region(Region.of(cfg.region()))
                .credentialsProvider(StaticCredentialsProvider.create(
                        AwsBasicCredentials.create(cfg.accessKey(), cfg.secretKey())))
                .serviceConfiguration(serviceConfig)
                .httpClientBuilder(ApacheHttpClient.builder()
                        .maxConnections(100)
                        .connectionTimeout(Duration.ofSeconds(3))
                        .socketTimeout(Duration.ofSeconds(20)))
                .overrideConfiguration(ClientOverrideConfiguration.builder()
                        // A save upload that cannot reach the store must fail
                        // quickly: the client retries on its own schedule and
                        // keeps the local save meanwhile. Hanging the request
                        // thread helps nobody.
                        .apiCallTimeout(Duration.ofSeconds(25))
                        .apiCallAttemptTimeout(Duration.ofSeconds(10))
                        .retryPolicy(RetryPolicy.builder()
                                .numRetries(3)
                                .retryCondition(RetryCondition.defaultRetryCondition())
                                .backoffStrategy(EqualJitterBackoffStrategy.builder()
                                        .baseDelay(Duration.ofMillis(120))
                                        .maxBackoffTime(Duration.ofSeconds(3))
                                        .build())
                                .build())
                        .build())
                .build();
    }

    /**
     * Presigner for direct client uploads/downloads.
     *
     * <p>Katta save blob'lari (8 MiB gacha) backend orqali oqib o'tishi shart
     * emas. Presigned URL bilan klient to'g'ridan object store bilan gaplashadi,
     * backend esa faqat metadata va imzo bilan shug'ullanadi. Bu — servisning
     * eng katta trafik tejamkorligi.
     */
    @Bean
    public S3Presigner s3Presigner() {
        ErtugrulProperties.S3 cfg = props.s3();

        return S3Presigner.builder()
                .endpointOverride(URI.create(cfg.endpoint()))
                .region(Region.of(cfg.region()))
                .credentialsProvider(StaticCredentialsProvider.create(
                        AwsBasicCredentials.create(cfg.accessKey(), cfg.secretKey())))
                .serviceConfiguration(S3Configuration.builder()
                        .pathStyleAccessEnabled(cfg.pathStyleAccess())
                        .build())
                .build();
    }
}
