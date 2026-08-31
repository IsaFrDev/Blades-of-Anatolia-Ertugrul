package com.fayzinc.ertugrul.identity;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.identity.dto.DeviceLoginRequest;
import com.fayzinc.ertugrul.identity.dto.LinkEntitlementRequest;
import com.fayzinc.ertugrul.identity.dto.PlayerProfileResponse;
import com.fayzinc.ertugrul.identity.dto.RefreshRequest;
import com.fayzinc.ertugrul.identity.dto.TokenResponse;
import com.fayzinc.ertugrul.identity.dto.UpdateProfileRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.dao.DataIntegrityViolationException;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.time.Instant;
import java.util.HexFormat;
import java.util.List;
import java.util.Optional;
import java.util.UUID;

/**
 * Account lifecycle: silent device sign-in, token rotation, store linking,
 * profile updates, and GDPR erasure.
 */
@Service
public class AuthService {

    private static final Logger log = LoggerFactory.getLogger(AuthService.class);

    private final PlayerRepository playerRepository;
    private final PlayerDeviceRepository deviceRepository;
    private final RefreshTokenRepository refreshTokenRepository;
    private final EntitlementRepository entitlementRepository;
    private final EntitlementVerifier entitlementVerifier;
    private final PlayerRegistrar playerRegistrar;
    private final JwtService jwtService;

    public AuthService(PlayerRepository playerRepository,
                       PlayerDeviceRepository deviceRepository,
                       RefreshTokenRepository refreshTokenRepository,
                       EntitlementRepository entitlementRepository,
                       EntitlementVerifier entitlementVerifier,
                       PlayerRegistrar playerRegistrar,
                       JwtService jwtService) {
        this.playerRepository = playerRepository;
        this.deviceRepository = deviceRepository;
        this.refreshTokenRepository = refreshTokenRepository;
        this.entitlementRepository = entitlementRepository;
        this.entitlementVerifier = entitlementVerifier;
        this.playerRegistrar = playerRegistrar;
        this.jwtService = jwtService;
    }

    /**
     * Qurilmaga bog'langan jimgina kirish — o'yin qiladigan birinchi tarmoq
     * chaqiruvi.
     *
     * <p>Agar qurilma tanish bo'lsa — mavjud akkaunt qaytariladi. Aks holda
     * <b>yangi anonim akkaunt yaratiladi</b>: o'yinchidan hech narsa
     * so'ralmaydi, ro'yxatdan o'tish oynasi ko'rsatilmaydi. Bu — mahsulot
     * qarori: birinchi daqiqadagi har qanday to'siq o'yinchini yo'qotadi.
     *
     * <p>Poyga holati (race): ikki parallel so'rov bir xil yangi
     * {@code deviceId} bilan kelsa, {@code uq_device_id} cheklovi ikkinchisini
     * rad etadi. Bu holat xato emas — biz shunchaki qayta o'qiymiz va mavjud
     * akkauntni qaytaramiz.
     *
     * @param request device, platform, and client build
     * @return a fresh token pair; {@code newAccount} tells the client whether to
     *         run first-run setup
     */
    @Transactional
    public TokenResponse deviceLogin(DeviceLoginRequest request) {
        Optional<PlayerDevice> existing = deviceRepository.findByDeviceId(request.deviceId());

        Player player;
        boolean newAccount = false;

        if (existing.isPresent()) {
            PlayerDevice device = existing.get();
            device.touch(request.appVersion());
            player = device.getPlayer();
        } else {
            try {
                // Separate bean -> real proxy boundary -> REQUIRES_NEW actually
                // applies, so a lost race rolls back only the inner transaction.
                player = playerRegistrar.createAccountForDevice(request);
                newAccount = true;
            } catch (DataIntegrityViolationException race) {
                // Lost the race — the other request created it. Read it back.
                log.debug("Concurrent device registration for {}, reusing existing", request.deviceId());
                PlayerDevice device = deviceRepository.findByDeviceId(request.deviceId())
                        .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.INTERNAL,
                                "Device vanished after unique-constraint violation", race));
                device.touch(request.appVersion());
                player = device.getPlayer();
            }
        }

        assertUsable(player);
        player.touchLastSeen();

        return issueTokenPair(player, request.deviceId(), UUID.randomUUID(), newAccount);
    }

    /**
     * Refresh token'ni rotatsiya qiladi.
     *
     * <p><b>Reuse detection.</b> Agar allaqachon ishlatilgan token kelsa —
     * yo u o'g'irlangan, yo klient javobni olmay qayta urinmoqda. Ikkalasini
     * ajratib bo'lmaydi, shuning uchun butun oila bekor qilinadi va o'yinchi
     * qaytadan kiradi. Bir martalik noqulaylik — o'g'irlangan sessiyaning
     * cheksiz davom etishidan afzal.
     *
     * @param request the token being redeemed plus its originating device
     * @return a new pair; the old token is now dead
     * @throws ErtugrulException {@code REFRESH_TOKEN_REUSED} when theft is suspected
     */
    @Transactional
    public TokenResponse refresh(RefreshRequest request) {
        String hash = sha256Hex(request.refreshToken());
        Instant now = Instant.now();

        RefreshToken stored = refreshTokenRepository.findByTokenHash(hash)
                .orElseThrow(() -> new ErtugrulException(
                        ErtugrulException.Code.REFRESH_TOKEN_INVALID, "Unknown refresh token"));

        if (stored.isAlreadyUsed()) {
            int revoked = refreshTokenRepository.revokeFamily(
                    stored.getFamilyId(), now, "reuse_detected");
            log.warn("Refresh token reuse detected for player={} family={}; revoked {} tokens",
                    stored.getPlayer().getId(), stored.getFamilyId(), revoked);
            throw new ErtugrulException(ErtugrulException.Code.REFRESH_TOKEN_REUSED,
                    "Refresh token was already used; all sessions revoked");
        }

        if (!stored.isUsable(now)) {
            throw new ErtugrulException(ErtugrulException.Code.REFRESH_TOKEN_INVALID,
                    "Refresh token is expired or revoked");
        }

        // Binding the token to its device stops a stolen token from being
        // redeemed from somewhere else.
        if (!stored.getDeviceId().equals(request.deviceId())) {
            log.warn("Refresh token device mismatch: issued to {}, presented by {}",
                    stored.getDeviceId(), request.deviceId());
            throw new ErtugrulException(ErtugrulException.Code.REFRESH_TOKEN_INVALID,
                    "Refresh token does not belong to this device");
        }

        stored.markUsed();

        Player player = stored.getPlayer();
        assertUsable(player);
        player.touchLastSeen();

        // Same family: this is a rotation, not a new login.
        return issueTokenPair(player, stored.getDeviceId(), stored.getFamilyId(), false);
    }

    /**
     * Do'kon akkauntini bog'laydi (Steam/PSN/Xbox).
     *
     * <p>Bu — anonim qurilma akkauntini <i>tiklanadigan</i> qiladigan qadam.
     * Klient yuborgan akkaunt ID'siga ishonilmaydi: haqiqiy identifikator
     * faqat {@link EntitlementVerifier} tekshiruvidan chiqadi. Tekshiruv
     * hozircha stub bo'lgani uchun yozuv {@code UNVERIFIED} holatida saqlanadi
     * va keyinroq davriy job uni tasdiqlaydi.
     *
     * @param playerId current player
     * @param request  provider and one-time platform ticket
     * @return the updated profile
     * @throws ErtugrulException {@code ENTITLEMENT_ALREADY_LINKED} if the store
     *         account belongs to a different player
     */
    @Transactional
    public PlayerProfileResponse linkEntitlement(UUID playerId, LinkEntitlementRequest request) {
        Player player = requirePlayer(playerId);

        Optional<EntitlementVerifier.VerifiedAccount> verified =
                entitlementVerifier.verify(request.provider(), request.sessionTicket(), request.productSku());

        // Fall back to the client's claim only while verification is stubbed.
        // Once EntitlementVerifier is real, an unverified ticket must be a hard
        // failure instead — see TODO(FAYZ-214).
        String accountId = verified
                .map(EntitlementVerifier.VerifiedAccount::accountId)
                .orElse(request.claimedAccountId());

        if (accountId == null || accountId.isBlank()) {
            throw new ErtugrulException(ErtugrulException.Code.ENTITLEMENT_INVALID,
                    "Could not determine a store account id");
        }

        Optional<Entitlement> conflicting = entitlementRepository
                .findByProviderAndProviderAccountIdAndProductSku(
                        request.provider(), accountId, request.productSku());

        if (conflicting.isPresent()) {
            Entitlement e = conflicting.get();
            if (!e.getPlayer().getId().equals(playerId)) {
                throw new ErtugrulException(ErtugrulException.Code.ENTITLEMENT_ALREADY_LINKED,
                        "This store account is already linked to another player");
            }
            // Idempotent re-link of the player's own entitlement.
            verified.filter(EntitlementVerifier.VerifiedAccount::ownsSku)
                    .ifPresent(v -> e.markVerified(entitlementVerifier.nextRevalidation()));
            return profileOf(player);
        }

        Entitlement entitlement = new Entitlement(
                player, request.provider(), accountId, request.productSku());
        verified.filter(EntitlementVerifier.VerifiedAccount::ownsSku)
                .ifPresent(v -> entitlement.markVerified(entitlementVerifier.nextRevalidation()));

        entitlementRepository.save(entitlement);
        log.info("Linked {} account to player={} (status={})",
                request.provider(), playerId, entitlement.getStatus());

        return profileOf(player);
    }

    /**
     * Profilni qisman yangilaydi. {@code null} maydon — "o'zgartirilmasin".
     *
     * <p>Telemetriya roziligi {@code OFF} ga o'tsa, hodisalar shu zahotiyoq
     * ingest chegarasida tashlanadi — keyinchalik filtrlash emas, umuman
     * Kafka'ga yozilmaydi (GDPR).
     */
    @Transactional
    public PlayerProfileResponse updateProfile(UUID playerId, UpdateProfileRequest request) {
        Player player = requirePlayer(playerId);

        if (request.displayName() != null) {
            player.setDisplayName(request.displayName().isBlank() ? null : request.displayName().trim());
        }
        if (request.locale() != null) {
            player.setLocale(request.locale());
        }
        if (request.telemetryConsent() != null) {
            player.setTelemetryConsent(request.telemetryConsent());
        }
        if (request.cloudSaveEnabled() != null) {
            player.setCloudSaveEnabled(request.cloudSaveEnabled());
        }
        if (request.codexSyncEnabled() != null) {
            player.setCodexSyncEnabled(request.codexSyncEnabled());
        }

        return profileOf(player);
    }

    @Transactional(readOnly = true)
    public PlayerProfileResponse getProfile(UUID playerId) {
        return profileOf(requirePlayer(playerId));
    }

    /** Revokes every live session for this player (sign out on all devices). */
    @Transactional
    public void logoutAll(UUID playerId) {
        int revoked = refreshTokenRepository.revokeAllForPlayer(playerId, Instant.now(), "logout_all");
        log.info("Revoked {} refresh tokens for player={}", revoked, playerId);
    }

    /**
     * GDPR o'chirish so'rovi (Settings → Akkaunt → Ma'lumotlarimni o'chirish).
     *
     * <p>Bu yerda darhol o'chirilmaydi: akkaunt {@code ERASURE_PENDING}
     * holatiga o'tadi, barcha sessiyalar bekor qilinadi, so'ng tungi job
     * shaxsiy ma'lumotni tozalaydi va save blob'larini object store'dan
     * o'chiradi. Kechiktirilgan o'chirish — tasodifan bosilgan tugmani
     * qaytarish imkonini beradi va sertifikatsiya talabiga ham mos keladi.
     */
    @Transactional
    public void requestErasure(UUID playerId) {
        Player player = requirePlayer(playerId);
        player.requestErasure();
        refreshTokenRepository.revokeAllForPlayer(playerId, Instant.now(), "gdpr_erasure");
        log.info("GDPR erasure requested for player={}", playerId);
    }

    // ── internals ───────────────────────────────────────────────────────────

    private TokenResponse issueTokenPair(Player player, String deviceId, UUID familyId, boolean newAccount) {
        JwtService.IssuedToken access = jwtService.issueAccessToken(player, deviceId, List.of());

        String rawRefresh = jwtService.generateRefreshToken();
        Instant refreshExpiry = jwtService.refreshTokenExpiry();

        refreshTokenRepository.save(new RefreshToken(
                player, deviceId, sha256Hex(rawRefresh), familyId, refreshExpiry));

        return TokenResponse.of(access.token(), rawRefresh, access.expiresAt(),
                refreshExpiry, player.getId(), newAccount);
    }

    private PlayerProfileResponse profileOf(Player player) {
        List<Entitlement> entitlements = entitlementRepository.findByPlayerId(player.getId());
        return PlayerProfileResponse.from(player, entitlements);
    }

    private Player requirePlayer(UUID playerId) {
        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));
        assertUsable(player);
        return player;
    }

    private void assertUsable(Player player) {
        if (player.getStatus() == Player.PlayerStatus.SUSPENDED) {
            throw new ErtugrulException(ErtugrulException.Code.ACCOUNT_SUSPENDED, "Account is suspended");
        }
        if (player.getStatus() == Player.PlayerStatus.ERASED) {
            throw new ErtugrulException(ErtugrulException.Code.PLAYER_NOT_FOUND, "Account was erased");
        }
    }

    /** SHA-256 hex. Refresh tokens are only ever persisted in this form. */
    static String sha256Hex(String input) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return HexFormat.of().formatHex(digest.digest(input.getBytes(StandardCharsets.UTF_8)));
        } catch (NoSuchAlgorithmException e) {
            throw new IllegalStateException("SHA-256 unavailable", e);
        }
    }
}
