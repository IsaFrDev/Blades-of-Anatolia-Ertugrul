package com.fayzinc.ertugrul.config;

import io.swagger.v3.oas.models.Components;
import io.swagger.v3.oas.models.OpenAPI;
import io.swagger.v3.oas.models.info.Contact;
import io.swagger.v3.oas.models.info.Info;
import io.swagger.v3.oas.models.info.License;
import io.swagger.v3.oas.models.security.SecurityRequirement;
import io.swagger.v3.oas.models.security.SecurityScheme;
import io.swagger.v3.oas.models.servers.Server;
import io.swagger.v3.oas.models.tags.Tag;
import org.springdoc.core.models.GroupedOpenApi;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import java.util.List;

/**
 * OpenAPI document for the meta-services API.
 *
 * <p>Bu spetsifikatsiya faqat hujjat emas — undan UE5 klienti uchun C++ DTO
 * struct'lari generatsiya qilinadi. Shuning uchun har bir DTO'da aniq tur va
 * misol bo'lishi muhim: generator {@code Object} turini ko'rsa, klient tomonda
 * qo'lda yozilgan parsing paydo bo'ladi va u albatta serverdan orqada qoladi.
 */
@Configuration
public class OpenApiConfig {

    private static final String BEARER_SCHEME = "bearerAuth";

    @Bean
    public OpenAPI ertugrulOpenApi() {
        return new OpenAPI()
                .info(new Info()
                        .title("Diriliş: The Last March — Meta Services API")
                        .version("v1")
                        .description("""
                                Online meta-services for a single-player UE5 title.

                                Scope: accounts, cloud saves, codex sync, the Safar Daftari
                                journey log, telemetry ingest, live-ops configuration, and
                                aggregate choice statistics.

                                Explicitly out of scope: gameplay netcode, matchmaking, PvP.
                                The game is fully playable with this service unreachable —
                                every endpoint is an enhancement, never a gate.
                                """)
                        .contact(new Contact()
                                .name("Fayz Inc. — Backend Team")
                                .email("backend@dirilis-game.com"))
                        .license(new License().name("Proprietary")))
                .servers(List.of(
                        new Server().url("https://api.dirilis-game.com").description("Production"),
                        new Server().url("https://api.stg.dirilis-game.com").description("Staging"),
                        new Server().url("http://localhost:8080").description("Local")))
                .tags(List.of(
                        new Tag().name("Auth").description("Device-linked accounts, JWT, store entitlements"),
                        new Tag().name("Saves").description("Versioned cloud saves; vector-clock conflict resolution"),
                        new Tag().name("Codex").description("~180 historical entries, cross-device union sync"),
                        new Tag().name("Journey").description("Safar Daftari diary: sync, PDF export, public share"),
                        new Tag().name("Telemetry").description("Funnel, wound balance, and choice events"),
                        new Tag().name("Live-ops").description("Remote config and per-episode balance overrides"),
                        new Tag().name("Stats").description("Aggregate choice distribution — not a leaderboard")))
                .components(new Components().addSecuritySchemes(BEARER_SCHEME,
                        new SecurityScheme()
                                .type(SecurityScheme.Type.HTTP)
                                .scheme("bearer")
                                .bearerFormat("JWT")
                                .description("Access token from POST /api/v1/auth/device or /auth/refresh")))
                .addSecurityItem(new SecurityRequirement().addList(BEARER_SCHEME));
    }

    /**
     * The surface the shipped game client talks to. Code generation for the UE5
     * client runs against this group only, so an accidentally public admin
     * endpoint cannot leak into the client SDK.
     */
    @Bean
    public GroupedOpenApi clientApi() {
        return GroupedOpenApi.builder()
                .group("client")
                .pathsToMatch("/api/v1/**")
                .pathsToExclude("/api/v1/liveops/admin/**")
                .build();
    }

    /** Internal live-ops dashboard surface. */
    @Bean
    public GroupedOpenApi adminApi() {
        return GroupedOpenApi.builder()
                .group("admin")
                .pathsToMatch("/api/v1/liveops/admin/**")
                .build();
    }
}
