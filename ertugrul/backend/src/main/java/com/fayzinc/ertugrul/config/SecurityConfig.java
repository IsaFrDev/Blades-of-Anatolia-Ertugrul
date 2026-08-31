package com.fayzinc.ertugrul.config;

import com.fayzinc.ertugrul.integrity.RateLimitFilter;
import com.nimbusds.jose.jwk.JWKSet;
import com.nimbusds.jose.jwk.RSAKey;
import com.nimbusds.jose.jwk.source.ImmutableJWKSet;
import com.nimbusds.jose.jwk.source.JWKSource;
import com.nimbusds.jose.proc.SecurityContext;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.http.HttpMethod;
import org.springframework.security.config.Customizer;
import org.springframework.security.config.annotation.method.configuration.EnableMethodSecurity;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.oauth2.jose.jws.SignatureAlgorithm;
import org.springframework.security.oauth2.jwt.JwtDecoder;
import org.springframework.security.oauth2.jwt.JwtEncoder;
import org.springframework.security.oauth2.jwt.NimbusJwtDecoder;
import org.springframework.security.oauth2.jwt.NimbusJwtEncoder;
import org.springframework.security.oauth2.server.resource.web.authentication.BearerTokenAuthenticationFilter;
import org.springframework.security.web.SecurityFilterChain;
import org.springframework.web.cors.CorsConfiguration;
import org.springframework.web.cors.CorsConfigurationSource;
import org.springframework.web.cors.UrlBasedCorsConfigurationSource;

import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.Base64;
import java.util.List;
import java.util.UUID;

/**
 * HTTP security for the meta-services API.
 *
 * <p><b>Model.</b> Server holatsiz (stateless): cookie yo'q, session yo'q, faqat
 * Bearer JWT. Sabab — klient konsol va PC'dagi o'yin binary'si, brauzer emas;
 * unda CSRF hujum yuzasi yo'q, lekin sessiya holati serverda saqlansa
 * gorizontal masshtablash qiyinlashadi.
 *
 * <p><b>Ochiq (permitAll) endpoint'lar ataylab juda kam:</b>
 * <ul>
 *   <li>{@code /auth/device}, {@code /auth/refresh} — token olishning o'zi</li>
 *   <li>{@code /journey/shared/**} — Safar Daftari ommaviy havolasi. Bu marketing
 *       xususiyati: havolani ochgan odam o'yinni sotib olmagan bo'lishi mumkin</li>
 *   <li>health/prometheus — infra probe'lari</li>
 * </ul>
 */
@Configuration
@EnableMethodSecurity
public class SecurityConfig {

    private static final Logger log = LoggerFactory.getLogger(SecurityConfig.class);

    private final ErtugrulProperties props;
    private final RateLimitFilter rateLimitFilter;

    public SecurityConfig(ErtugrulProperties props, RateLimitFilter rateLimitFilter) {
        this.props = props;
        this.rateLimitFilter = rateLimitFilter;
    }

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http) throws Exception {
        http
                // No cookies, no browser forms -> CSRF protection is dead weight.
                .csrf(csrf -> csrf.disable())
                .cors(cors -> cors.configurationSource(corsConfigurationSource()))
                .sessionManagement(sm -> sm.sessionCreationPolicy(SessionCreationPolicy.STATELESS))
                .authorizeHttpRequests(auth -> auth
                        // ── Token acquisition ───────────────────────────────
                        .requestMatchers(HttpMethod.POST,
                                "/api/v1/auth/device",
                                "/api/v1/auth/refresh").permitAll()

                        // ── Public Safar Daftari share pages ────────────────
                        // Read-only, token-gated, exposes diary text only.
                        .requestMatchers(HttpMethod.GET, "/api/v1/journey/shared/**").permitAll()

                        // ── Infra ───────────────────────────────────────────
                        .requestMatchers("/actuator/health/**", "/actuator/info").permitAll()
                        .requestMatchers("/actuator/prometheus", "/actuator/metrics/**")
                            .hasAuthority("SCOPE_ops")
                        .requestMatchers("/v3/api-docs/**", "/swagger-ui/**", "/swagger-ui.html")
                            .permitAll()

                        // ── Live-ops authoring is staff-only ────────────────
                        .requestMatchers("/api/v1/liveops/admin/**").hasAuthority("SCOPE_liveops:write")

                        .anyRequest().authenticated())

                .oauth2ResourceServer(oauth -> oauth.jwt(Customizer.withDefaults()))

                // Rate limiting runs AFTER authentication so buckets can be keyed
                // by player id rather than by IP. Console players behind carrier
                // NAT share an IP; keying on IP would throttle innocent players.
                .addFilterAfter(rateLimitFilter, BearerTokenAuthenticationFilter.class);

        return http.build();
    }

    /**
     * The RSA keypair used to sign and verify access tokens, as a single bean so
     * the encoder and decoder cannot drift apart.
     *
     * <p>Production'da PEM secret manager'dan keladi. Agar berilmagan bo'lsa —
     * <b>faqat dev uchun</b> — vaqtinchalik kalit generatsiya qilinadi. Bu
     * holatda server qayta ishga tushsa barcha access token'lar yaroqsiz
     * bo'ladi, lekin refresh token'lar DB'da qolgani uchun klient jimgina
     * qayta tiklaydi — o'yinchi buni sezmaydi.
     *
     * <p>Key rotation: publish two keys in the JWKS and sign with the newer one;
     * the decoder accepts both until every issued token has expired.
     */
    @Bean
    public RSAKey rsaSigningKey() {
        String privatePem = props.security().jwt().privateKeyPem();
        String publicPem = props.security().jwt().publicKeyPem();

        RSAPublicKey publicKey;
        RSAPrivateKey privateKey;

        if (privatePem != null && !privatePem.isBlank() && publicPem != null && !publicPem.isBlank()) {
            publicKey = readPublicKey(publicPem);
            privateKey = readPrivateKey(privatePem);
            log.info("JWT signing key loaded from configuration");
        } else {
            KeyPair generated = generateEphemeralKeyPair();
            publicKey = (RSAPublicKey) generated.getPublic();
            privateKey = (RSAPrivateKey) generated.getPrivate();
            log.warn("""
                    No RSA keypair configured (ertugrul.security.jwt.private-key-pem). \
                    Generated an EPHEMERAL keypair - all access tokens become invalid on \
                    restart. This is acceptable for local development ONLY.""");
        }

        return new RSAKey.Builder(publicKey)
                .privateKey(privateKey)
                .keyID(UUID.randomUUID().toString())
                .build();
    }

    @Bean
    public JWKSource<SecurityContext> jwkSource(RSAKey rsaSigningKey) {
        return new ImmutableJWKSet<>(new JWKSet(rsaSigningKey));
    }

    @Bean
    public JwtEncoder jwtEncoder(JWKSource<SecurityContext> jwkSource) {
        return new NimbusJwtEncoder(jwkSource);
    }

    /**
     * Decoder validates signature and expiry. Issuer and audience are asserted
     * explicitly in {@code JwtService}, because the resource server's default
     * validator checks neither unless told to.
     */
    @Bean
    public JwtDecoder jwtDecoder(RSAKey rsaSigningKey) throws Exception {
        return NimbusJwtDecoder
                .withPublicKey(rsaSigningKey.toRSAPublicKey())
                .signatureAlgorithm(SignatureAlgorithm.RS256)
                .build();
    }

    /**
     * CORS exists only for the Safar Daftari share page served from the
     * marketing site. The game client is not a browser and ignores CORS.
     */
    @Bean
    public CorsConfigurationSource corsConfigurationSource() {
        CorsConfiguration config = new CorsConfiguration();
        config.setAllowedOrigins(List.of(
                "https://dirilis-game.com",
                "https://www.dirilis-game.com"));
        config.setAllowedMethods(List.of("GET", "OPTIONS"));
        config.setAllowedHeaders(List.of("Content-Type", "Accept"));
        config.setMaxAge(3600L);

        UrlBasedCorsConfigurationSource source = new UrlBasedCorsConfigurationSource();
        source.registerCorsConfiguration("/api/v1/journey/shared/**", config);
        return source;
    }

    // ── PEM helpers ─────────────────────────────────────────────────────────

    private static RSAPublicKey readPublicKey(String pem) {
        try {
            byte[] der = Base64.getMimeDecoder().decode(stripPemArmour(pem));
            return (RSAPublicKey) KeyFactory.getInstance("RSA")
                    .generatePublic(new X509EncodedKeySpec(der));
        } catch (Exception e) {
            throw new IllegalStateException("Invalid RSA public key PEM", e);
        }
    }

    private static RSAPrivateKey readPrivateKey(String pem) {
        try {
            byte[] der = Base64.getMimeDecoder().decode(stripPemArmour(pem));
            return (RSAPrivateKey) KeyFactory.getInstance("RSA")
                    .generatePrivate(new PKCS8EncodedKeySpec(der));
        } catch (Exception e) {
            throw new IllegalStateException("Invalid RSA private key PEM", e);
        }
    }

    private static String stripPemArmour(String pem) {
        return pem
                .replaceAll("-----BEGIN (.*)-----", "")
                .replaceAll("-----END (.*)-----", "")
                .replaceAll("\\s", "");
    }

    private static KeyPair generateEphemeralKeyPair() {
        try {
            KeyPairGenerator generator = KeyPairGenerator.getInstance("RSA");
            generator.initialize(2048);
            return generator.generateKeyPair();
        } catch (Exception e) {
            throw new IllegalStateException("Cannot generate RSA keypair", e);
        }
    }
}
