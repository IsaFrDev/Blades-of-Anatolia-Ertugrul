package com.fayzinc.ertugrul;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.ConfigurationPropertiesScan;
import org.springframework.data.jpa.repository.config.EnableJpaAuditing;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.scheduling.annotation.EnableScheduling;

/**
 * "Diriliş: The Last March" — online meta-services backend.
 *
 * <p><b>Loyiha haqida.</b> O'yinning o'zi to'liq <i>single-player</i>: bu servis
 * gameplay netcode emas, faqat <i>meta-xizmatlar</i> beradi — akkaunt, bulutli
 * saqlash, kodeks sinxroni, Safar Daftari, telemetriya, live-ops va integrity.
 * Ya'ni server ishlamay qolsa ham o'yinchi o'ynay oladi; u faqat qurilmalararo
 * sinxron va live-ops balansini yo'qotadi. Bu — arxitekturaning asosiy qoidasi:
 * <b>backend hech qachon o'yinni to'xtatmaydi</b> (offline-first).
 *
 * <p>Modules:
 * <ul>
 *   <li>{@code identity} — device-linked accounts, JWT, store entitlements</li>
 *   <li>{@code save}     — versioned cloud saves, vector-clock conflict resolution</li>
 *   <li>{@code codex}    — ~180 historical entries, cross-device union sync</li>
 *   <li>{@code journey}  — Safar Daftari diary, PDF export, public share links</li>
 *   <li>{@code telemetry}— Kafka ingest: episode funnel, wound balance, choices</li>
 *   <li>{@code liveops}  — remote config, per-episode balance overrides, events</li>
 *   <li>{@code stats}    — "how did your choices compare" aggregates</li>
 *   <li>{@code integrity}— save HMAC signing, telemetry sanity, rate limiting</li>
 * </ul>
 */
@SpringBootApplication
@ConfigurationPropertiesScan
@EnableJpaAuditing          // populates Auditable.createdAt / updatedAt
@EnableScheduling           // rollup flush, export worker, token & blob pruning
@EnableAsync                // telemetry produce + PDF generation off the request thread
public class ErtugrulBackendApplication {

    public static void main(String[] args) {
        SpringApplication.run(ErtugrulBackendApplication.class, args);
    }
}
