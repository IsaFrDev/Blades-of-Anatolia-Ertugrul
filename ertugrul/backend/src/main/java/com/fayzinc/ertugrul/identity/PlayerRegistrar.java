package com.fayzinc.ertugrul.identity;

import com.fayzinc.ertugrul.identity.dto.DeviceLoginRequest;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;

/**
 * Creates a brand-new account and its first device.
 *
 * <p><b>Nega alohida komponent.</b> Yangi akkaunt yaratish o'z tranzaksiyasida
 * ({@code REQUIRES_NEW}) bajarilishi kerak: ikki parallel so'rov bir xil yangi
 * {@code deviceId} bilan kelsa, biri {@code uq_device_id} cheklovini buzadi va
 * <i>faqat o'sha ichki tranzaksiya</i> orqaga qaytishi lozim — tashqi so'rov
 * esa mavjud akkauntni o'qib davom etadi.
 *
 * <p>Agar bu metod {@code AuthService} ichida qolganida, Spring'ning proxy'si
 * chetlab o'tilardi (self-invocation) va {@code REQUIRES_NEW} <b>umuman
 * ishlamasdi</b> — tashqi tranzaksiya {@code rollback-only} belgisini olib,
 * butun so'rov qulardi. Shuning uchun bu alohida bean.
 */
@Component
public class PlayerRegistrar {

    private static final Logger log = LoggerFactory.getLogger(PlayerRegistrar.class);

    private final PlayerRepository playerRepository;
    private final PlayerDeviceRepository deviceRepository;

    public PlayerRegistrar(PlayerRepository playerRepository,
                           PlayerDeviceRepository deviceRepository) {
        this.playerRepository = playerRepository;
        this.deviceRepository = deviceRepository;
    }

    /**
     * Yangi anonim akkaunt va uning birinchi qurilmasini yaratadi.
     *
     * @param request the originating device login
     * @return the freshly created player
     * @throws org.springframework.dao.DataIntegrityViolationException when a
     *         concurrent request registered the same device first; the caller is
     *         expected to catch this and read the existing account back
     */
    @Transactional(propagation = Propagation.REQUIRES_NEW)
    public Player createAccountForDevice(DeviceLoginRequest request) {
        Player player = playerRepository.saveAndFlush(new Player(request.locale()));

        // saveAndFlush so the unique-constraint violation surfaces here, inside
        // this inner transaction, rather than at outer commit time where it
        // could no longer be recovered from.
        deviceRepository.saveAndFlush(new PlayerDevice(
                player, request.deviceId(), request.platform(), request.appVersion()));

        log.info("Created anonymous account player={} for device={} platform={}",
                player.getId(), request.deviceId(), request.platform());
        return player;
    }
}
