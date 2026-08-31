package com.fayzinc.ertugrul.identity;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import org.springframework.security.core.Authentication;
import org.springframework.security.oauth2.jwt.JwsHeader;
import org.springframework.security.oauth2.jwt.Jwt;
import org.springframework.security.oauth2.jwt.JwtClaimsSet;
import org.springframework.security.oauth2.jwt.JwtEncoder;
import org.springframework.security.oauth2.jwt.JwtEncoderParameters;
import org.springframework.stereotype.Service;

import java.security.SecureRandom;
import java.time.Instant;
import java.util.Base64;
import java.util.List;
import java.util.UUID;

/**
 * Issues access tokens and generates opaque refresh tokens.
 *
 * <p><b>Token dizayni.</b> Access token — RS256 JWT, ichida faqat serverga
 * kerakli minimal ma'lumot: kim (subject), qaysi qurilmadan, qanday huquqlar.
 * Unga <i>hech qanday o'yin holati</i> (epizod, jarohat, progress) solinmaydi —
 * bu ma'lumot tez o'zgaradi va token 30 daqiqa yashaydi, ya'ni u darhol
 * eskiradi va noto'g'ri qaror manbaiga aylanadi.
 *
 * <p>Refresh token esa JWT emas — u shunchaki tasodifiy satr. Sabab: uni
 * <b>bekor qilish</b> mumkin bo'lishi kerak, JWT esa ta'rifi bo'yicha
 * bekor qilinmaydi.
 */
@Service
public class JwtService {

    /** Claim carrying the device the token was minted for. */
    public static final String CLAIM_DEVICE_ID = "did";
    /** Claim carrying the token's purpose, guarding against cross-use. */
    public static final String CLAIM_TOKEN_USE = "use";
    public static final String TOKEN_USE_ACCESS = "access";

    private static final SecureRandom RANDOM = new SecureRandom();
    private static final int REFRESH_TOKEN_BYTES = 32;   // 256 bits

    private final JwtEncoder jwtEncoder;
    private final ErtugrulProperties props;

    public JwtService(JwtEncoder jwtEncoder, ErtugrulProperties props) {
        this.jwtEncoder = jwtEncoder;
        this.props = props;
    }

    /**
     * Access token yaratadi.
     *
     * @param player   the authenticated player
     * @param deviceId device the token is bound to
     * @param scopes   granted scopes; ordinary players get none, staff get {@code liveops:write}
     * @return signed compact JWT and its expiry
     */
    public IssuedToken issueAccessToken(Player player, String deviceId, List<String> scopes) {
        Instant now = Instant.now();
        Instant expiresAt = now.plus(props.security().jwt().accessTokenTtl());

        JwtClaimsSet.Builder claims = JwtClaimsSet.builder()
                .issuer(props.security().jwt().issuer())
                .audience(List.of(props.security().jwt().audience()))
                .subject(player.getId().toString())
                .issuedAt(now)
                .expiresAt(expiresAt)
                // jti lets us blacklist a single leaked token without waiting
                // for expiry, should that ever be necessary.
                .id(UUID.randomUUID().toString())
                .claim(CLAIM_DEVICE_ID, deviceId)
                .claim(CLAIM_TOKEN_USE, TOKEN_USE_ACCESS);

        if (scopes != null && !scopes.isEmpty()) {
            // Space-delimited "scope" is what Spring's JwtGrantedAuthoritiesConverter
            // reads by default, turning each into a SCOPE_* authority.
            claims.claim("scope", String.join(" ", scopes));
        }

        JwsHeader header = JwsHeader.with(org.springframework.security.oauth2.jose.jws.SignatureAlgorithm.RS256)
                .build();

        String token = jwtEncoder.encode(JwtEncoderParameters.from(header, claims.build()))
                .getTokenValue();

        return new IssuedToken(token, expiresAt);
    }

    /**
     * Yangi refresh token generatsiya qiladi.
     *
     * <p>256 bit entropiya, URL-safe base64. Serverda faqat uning SHA-256
     * hash'i saqlanadi.
     *
     * @return the raw token — this is the only moment it exists in plaintext
     */
    public String generateRefreshToken() {
        byte[] bytes = new byte[REFRESH_TOKEN_BYTES];
        RANDOM.nextBytes(bytes);
        return "rt_" + Base64.getUrlEncoder().withoutPadding().encodeToString(bytes);
    }

    /** Refresh token expiry for a token minted now. */
    public Instant refreshTokenExpiry() {
        return Instant.now().plus(props.security().jwt().refreshTokenTtl());
    }

    /**
     * Extracts the player id from an authenticated request.
     *
     * @throws ErtugrulException if the principal is not a JWT or the subject is unparseable
     */
    public UUID requirePlayerId(Authentication authentication) {
        if (authentication == null || !(authentication.getPrincipal() instanceof Jwt jwt)) {
            throw new ErtugrulException(ErtugrulException.Code.INVALID_CREDENTIALS,
                    "No authenticated JWT principal");
        }
        assertAudience(jwt);
        try {
            return UUID.fromString(jwt.getSubject());
        } catch (IllegalArgumentException e) {
            throw new ErtugrulException(ErtugrulException.Code.INVALID_CREDENTIALS,
                    "Malformed subject claim", e);
        }
    }

    /** Extracts the device the caller's token was minted for. */
    public String requireDeviceId(Authentication authentication) {
        if (authentication == null || !(authentication.getPrincipal() instanceof Jwt jwt)) {
            throw new ErtugrulException(ErtugrulException.Code.INVALID_CREDENTIALS,
                    "No authenticated JWT principal");
        }
        String deviceId = jwt.getClaimAsString(CLAIM_DEVICE_ID);
        if (deviceId == null || deviceId.isBlank()) {
            throw new ErtugrulException(ErtugrulException.Code.INVALID_CREDENTIALS,
                    "Token carries no device claim");
        }
        return deviceId;
    }

    /**
     * Spring's resource server does not verify {@code aud} by default, so it is
     * asserted here. Without this, a token minted for another audience by the
     * same key would be accepted.
     */
    private void assertAudience(Jwt jwt) {
        List<String> audience = jwt.getAudience();
        String expected = props.security().jwt().audience();
        if (audience == null || !audience.contains(expected)) {
            throw new ErtugrulException(ErtugrulException.Code.INVALID_CREDENTIALS,
                    "Token audience mismatch");
        }
    }

    /**
     * A freshly minted access token.
     *
     * @param token     compact serialised JWT
     * @param expiresAt when it stops being accepted
     */
    public record IssuedToken(String token, Instant expiresAt) {
    }
}
