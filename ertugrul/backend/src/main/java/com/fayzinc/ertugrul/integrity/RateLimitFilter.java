package com.fayzinc.ertugrul.integrity;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fayzinc.ertugrul.common.ApiError;
import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.config.RedisConfig;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.data.redis.core.script.RedisScript;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;

import java.io.IOException;
import java.time.Instant;
import java.util.List;

/**
 * Per-player, per-route-class rate limiting.
 *
 * <p><b>Nega o'yinchi bo'yicha, IP bo'yicha emas.</b> Konsol o'yinchilari
 * operator NAT'i ortida o'tiradi va minglab odam bitta tashqi IP'ni baham
 * ko'radi. IP bo'yicha cheklash — butun bir mintaqani bitta suiiste'molchi
 * tufayli bloklash degani. Shuning uchun filtr autentifikatsiyadan
 * <b>keyin</b> ishlaydi ({@code SecurityConfig} da
 * {@code BearerTokenAuthenticationFilter} dan keyin qo'yilgan) va
 * {@code playerId} ni kalit sifatida ishlatadi. IP faqat token hali yo'q
 * bo'lgan yo'llarda ({@code /auth/*}) zaxira sifatida qoladi.
 *
 * <p><b>Nega Redis + Lua.</b> {@code GET → tekshir → DECR} ketma-ketligi
 * atomik emas: ikki parallel so'rov chegaradan birga o'tib ketishi mumkin.
 * Lua skripti Redis'da bitta bo'linmas qadamda bajariladi.
 *
 * <p><b>Redis yiqilsa — so'rov o'tkaziladi.</b> Bu ataylab tanlangan
 * yo'nalish: rate limiter o'yinchini o'yindan to'sib qo'yadigan komponentga
 * aylanmasligi kerak. Suiiste'mol xavfi — vaqtincha; ishlamayotgan servis —
 * darhol ko'rinadigan zarar.
 */
@Component
public class RateLimitFilter extends OncePerRequestFilter {

    private static final Logger log = LoggerFactory.getLogger(RateLimitFilter.class);

    /** Fixed one-minute windows; the Lua script sets the TTL on first use. */
    private static final int WINDOW_SECONDS = 60;

    private final StringRedisTemplate redis;
    private final RedisScript<Long> tokenBucketScript;
    private final ErtugrulProperties props;
    private final ObjectMapper objectMapper;
    private final Counter throttled;

    public RateLimitFilter(StringRedisTemplate stringRedisTemplate,
                           RedisScript<Long> tokenBucketScript,
                           ErtugrulProperties props,
                           ObjectMapper objectMapper,
                           MeterRegistry meterRegistry) {
        this.redis = stringRedisTemplate;
        this.tokenBucketScript = tokenBucketScript;
        this.props = props;
        this.objectMapper = objectMapper;
        this.throttled = Counter.builder("ertugrul.ratelimit.throttled").register(meterRegistry);
    }

    @Override
    protected boolean shouldNotFilter(HttpServletRequest request) {
        String path = request.getRequestURI();
        // Infrastructure probes and the API docs are never rate limited: a
        // throttled health check would take the instance out of rotation.
        return path.startsWith("/actuator/")
                || path.startsWith("/v3/api-docs")
                || path.startsWith("/swagger-ui");
    }

    @Override
    protected void doFilterInternal(HttpServletRequest request,
                                    HttpServletResponse response,
                                    FilterChain filterChain)
            throws ServletException, IOException {

        if (!props.ratelimit().enabled()) {
            filterChain.doFilter(request, response);
            return;
        }

        Bucket bucket = bucketFor(request.getRequestURI());
        String subject = subjectFor(request);
        long window = Instant.now().getEpochSecond() / WINDOW_SECONDS;
        String key = RedisConfig.Keys.rateLimit(bucket.name, subject, window);

        Long remaining = consumeToken(key, bucket.capacity(props));

        if (remaining != null && remaining < 0) {
            throttled.increment();
            log.debug("Rate limited {} on bucket={} subject={}",
                    request.getRequestURI(), bucket.name, subject);
            writeTooManyRequests(request, response, bucket);
            return;
        }

        if (remaining != null) {
            response.setHeader("X-RateLimit-Limit", Integer.toString(bucket.capacity(props)));
            response.setHeader("X-RateLimit-Remaining", Long.toString(Math.max(remaining, 0)));
        }

        filterChain.doFilter(request, response);
    }

    /**
     * Bir token sarflaydi.
     *
     * @return remaining tokens, {@code -1} when exhausted, or {@code null} when
     *         Redis is unreachable — in which case the request is let through
     */
    private Long consumeToken(String key, int capacity) {
        try {
            return redis.execute(tokenBucketScript, List.of(key),
                    Integer.toString(capacity), Integer.toString(WINDOW_SECONDS));
        } catch (Exception e) {
            // Fail open. See the class javadoc: an unavailable limiter must not
            // become an unavailable service.
            log.warn("Rate limit check failed; allowing request", e);
            return null;
        }
    }

    /**
     * So'rov yo'liga qarab qaysi chelakka tegishli ekanini aniqlaydi.
     *
     * <p>Chegaralar juda har xil: autentifikatsiya eng qattiq (credential
     * stuffing xavfi), telemetriya eng yumshoq (halol klient har necha
     * soniyada paket yuboradi).
     */
    private static Bucket bucketFor(String path) {
        if (path.startsWith("/api/v1/auth/")) {
            return Bucket.AUTH;
        }
        if (path.startsWith("/api/v1/saves")) {
            return Bucket.SAVE;
        }
        if (path.startsWith("/api/v1/telemetry")) {
            return Bucket.TELEMETRY;
        }
        return Bucket.DEFAULT;
    }

    /**
     * Chelak kaliti: iloji bo'lsa o'yinchi ID'si, aks holda IP.
     *
     * <p>Token bo'lmagan yo'llarda ({@code /auth/device}) IP'dan boshqa
     * ishonchli identifikator yo'q — bu holat uchun cheklov ham qattiqroq
     * qo'yilgan.
     */
    private static String subjectFor(HttpServletRequest request) {
        Authentication authentication = SecurityContextHolder.getContext().getAuthentication();

        if (authentication != null
                && authentication.isAuthenticated()
                && authentication.getPrincipal() instanceof Jwt jwt
                && jwt.getSubject() != null) {
            return "p:" + jwt.getSubject();
        }
        return "ip:" + clientIp(request);
    }

    /**
     * Klient IP'si.
     *
     * <p>{@code X-Forwarded-For} ning <b>birinchi</b> qiymati olinadi. Bu
     * faqat ishonchli reverse proxy ortida to'g'ri ishlaydi — sarlavhani
     * klient o'zi ham yuborishi mumkin. Ishlab chiqarishda proxy uni
     * qayta yozadi, shuning uchun bu joyda qo'shimcha tekshiruv yo'q; agar
     * arxitektura o'zgarsa, bu metod ham qayta ko'rilishi kerak.
     */
    private static String clientIp(HttpServletRequest request) {
        String forwarded = request.getHeader("X-Forwarded-For");
        if (forwarded != null && !forwarded.isBlank()) {
            int comma = forwarded.indexOf(',');
            return (comma > 0 ? forwarded.substring(0, comma) : forwarded).trim();
        }
        return request.getRemoteAddr();
    }

    private void writeTooManyRequests(HttpServletRequest request,
                                      HttpServletResponse response,
                                      Bucket bucket) throws IOException {

        response.setStatus(HttpStatus.TOO_MANY_REQUESTS.value());
        response.setContentType(MediaType.APPLICATION_JSON_VALUE);
        response.setHeader(HttpHeaders.RETRY_AFTER, Integer.toString(WINDOW_SECONDS));

        ApiError error = ApiError.of(
                ErtugrulException.Code.RATE_LIMITED,
                "Rate limit exceeded for " + bucket.name,
                request.getRequestURI(),
                Long.toHexString(System.nanoTime()));

        objectMapper.writeValue(response.getOutputStream(), error);
    }

    /** Route classes, each with its own per-minute budget. */
    private enum Bucket {
        AUTH("auth"),
        SAVE("save"),
        TELEMETRY("telemetry"),
        DEFAULT("default");

        private final String name;

        Bucket(String name) {
            this.name = name;
        }

        int capacity(ErtugrulProperties props) {
            return switch (this) {
                case AUTH -> props.ratelimit().authPerMinute();
                case SAVE -> props.ratelimit().saveUploadPerMinute();
                case TELEMETRY -> props.ratelimit().telemetryPerMinute();
                case DEFAULT -> props.ratelimit().defaultPerMinute();
            };
        }
    }
}
