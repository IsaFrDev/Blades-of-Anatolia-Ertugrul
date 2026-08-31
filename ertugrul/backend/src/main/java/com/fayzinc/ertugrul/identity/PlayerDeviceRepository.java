package com.fayzinc.ertugrul.identity;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;
import java.util.UUID;

@Repository
public interface PlayerDeviceRepository extends JpaRepository<PlayerDevice, UUID> {

    Optional<PlayerDevice> findByDeviceId(String deviceId);

    List<PlayerDevice> findByPlayerId(UUID playerId);

    boolean existsByDeviceId(String deviceId);

    /**
     * Devices that have written to a save recently — the candidate set of
     * vector-clock nodes.
     *
     * <p>Uzoq vaqt ishlatilmagan qurilmalar vector clock'da "o'lik tugun" bo'lib
     * qoladi va soatni behuda kattalashtiradi. Ularni siqish (compaction) uchun
     * shu ro'yxat ishlatiladi.
     */
    List<PlayerDevice> findByPlayerIdOrderByLastSeenAtDesc(UUID playerId);
}
