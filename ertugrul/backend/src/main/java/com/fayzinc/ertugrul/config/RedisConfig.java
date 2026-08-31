package com.fayzinc.ertugrul.config;

import com.fasterxml.jackson.annotation.JsonAutoDetect;
import com.fasterxml.jackson.annotation.PropertyAccessor;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.data.redis.connection.RedisConnectionFactory;
import org.springframework.data.redis.core.RedisTemplate;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.data.redis.core.script.DefaultRedisScript;
import org.springframework.data.redis.core.script.RedisScript;
import org.springframework.data.redis.serializer.GenericJackson2JsonRedisSerializer;
import org.springframework.data.redis.serializer.StringRedisSerializer;

/**
 * Redis wiring: rate-limit buckets, remote-config cache, choice-stats cache.
 *
 * <p><b>Muhim qaror:</b> Redis'dagi hech bir ma'lumot <i>manba</i> emas — hammasi
 * Postgres yoki Kafka'dan qayta tiklanadi. Shuning uchun {@code docker-compose}
 * da persistence o'chirilgan va {@code allkeys-lru} siyosati qo'yilgan. Redis
 * yiqilsa servis sekinlashadi, lekin <b>ishlashdan to'xtamaydi</b> — bu
 * offline-first falsafasining server tomonidagi ko'rinishi.
 */
@Configuration
public class RedisConfig {

    /**
     * Atomic token-bucket refill + consume.
     *
     * <p>Nega Lua: GET/DECR/EXPIRE ketma-ketligi atomik emas — ikki parallel
     * so'rov limitdan o'tib ketishi mumkin. Lua skripti Redis'da bitta
     * bo'linmas qadamda bajariladi.
     *
     * <p>KEYS[1] = bucket key, ARGV[1] = capacity, ARGV[2] = window seconds.
     * Returns remaining tokens, or -1 when the bucket is exhausted.
     */
    private static final String TOKEN_BUCKET_LUA = """
            local key      = KEYS[1]
            local capacity = tonumber(ARGV[1])
            local window   = tonumber(ARGV[2])

            local current = redis.call('GET', key)
            if current == false then
                -- First request in this window: create the bucket.
                redis.call('SET', key, capacity - 1, 'EX', window)
                return capacity - 1
            end

            current = tonumber(current)
            if current <= 0 then
                return -1
            end

            -- DECR keeps the existing TTL, so the window is fixed, not sliding.
            return redis.call('DECR', key)
            """;

    @Bean
    public RedisScript<Long> tokenBucketScript() {
        DefaultRedisScript<Long> script = new DefaultRedisScript<>();
        script.setScriptText(TOKEN_BUCKET_LUA);
        script.setResultType(Long.class);
        return script;
    }

    /**
     * String template for counters and rate-limit keys — no serialisation
     * overhead where the value is a number.
     */
    @Bean
    public StringRedisTemplate stringRedisTemplate(RedisConnectionFactory factory) {
        return new StringRedisTemplate(factory);
    }

    /**
     * JSON template for cached documents (resolved remote config, choice
     * aggregate snapshots).
     *
     * <p>Type information is deliberately NOT embedded in the payload: cached
     * values are read back into known record types, and polymorphic type ids in
     * Redis are a deserialisation-gadget risk.
     */
    @Bean
    public RedisTemplate<String, Object> jsonRedisTemplate(RedisConnectionFactory factory) {
        ObjectMapper mapper = new ObjectMapper()
                .registerModule(new JavaTimeModule())
                .setVisibility(PropertyAccessor.ALL, JsonAutoDetect.Visibility.ANY);

        RedisTemplate<String, Object> template = new RedisTemplate<>();
        template.setConnectionFactory(factory);
        template.setKeySerializer(new StringRedisSerializer());
        template.setHashKeySerializer(new StringRedisSerializer());
        template.setValueSerializer(new GenericJackson2JsonRedisSerializer(mapper));
        template.setHashValueSerializer(new GenericJackson2JsonRedisSerializer(mapper));
        template.afterPropertiesSet();
        return template;
    }

    /**
     * Namespaced key builders. Centralised so a key format change cannot be
     * made in one place and forgotten in another.
     */
    public static final class Keys {

        private static final String PREFIX = "ert";

        private Keys() {
        }

        /** Rate-limit bucket, e.g. {@code ert:rl:save:<playerId>:<minute>}. */
        public static String rateLimit(String bucketClass, String subject, long windowIndex) {
            return String.join(":", PREFIX, "rl", bucketClass, subject, Long.toString(windowIndex));
        }

        /** Resolved remote config for one cohort bucket. */
        public static String remoteConfig(int cohortBucket, String appVersion) {
            return String.join(":", PREFIX, "cfg", Integer.toString(cohortBucket), appVersion);
        }

        /**
         * Glob matching every resolved remote-config entry.
         *
         * <p>Live-ops o'zgarish kiritganda butun hujjat keshi tozalanadi —
         * o'yinchilar TTL tugashini kutmasligi kerak. Naqsh shu yerda, kalit
         * quruvchi bilan yonma-yon: ular birga o'zgarishi shart.
         */
        public static String remoteConfigPattern() {
            return String.join(":", PREFIX, "cfg", "*");
        }

        /** Per-episode balance override document. */
        public static String balanceOverride(String episodeId) {
            return String.join(":", PREFIX, "balance", episodeId);
        }

        /** Cached choice split for one choice id. */
        public static String choiceStats(String choiceId) {
            return String.join(":", PREFIX, "choice", choiceId);
        }

        /** Share-link resolution cache: token -> playthrough. */
        public static String shareToken(String token) {
            return String.join(":", PREFIX, "share", token);
        }
    }
}
