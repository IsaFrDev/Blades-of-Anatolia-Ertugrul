package com.fayzinc.ertugrul.telemetry;

import com.fayzinc.ertugrul.identity.JwtService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.responses.ApiResponse;
import io.swagger.v3.oas.annotations.responses.ApiResponses;
import io.swagger.v3.oas.annotations.security.SecurityRequirement;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotEmpty;
import jakarta.validation.constraints.Size;
import org.springframework.http.ResponseEntity;
import org.springframework.security.core.Authentication;
import org.springframework.validation.annotation.Validated;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.UUID;

/**
 * Telemetry ingest.
 *
 * <p>Klient hodisalarni to'plab, davriy ravishda paket sifatida yuboradi.
 * Endpoint <b>hech qachon o'yinni kutdirmaydi</b>: tekshiruv sinxron, Kafka'ga
 * yozish esa asinxron, va javob doim {@code 202 Accepted}.
 *
 * <p>Butun mantiq {@link TelemetryIngestService} da — bu yerda faqat HTTP.
 */
@RestController
@RequestMapping("/api/v1/telemetry")
@Validated
@SecurityRequirement(name = "bearerAuth")
@Tag(name = "Telemetry", description = "Episode funnel, wound balance, and choice events")
public class TelemetryController {

    private final TelemetryIngestService ingestService;
    private final JwtService jwtService;

    public TelemetryController(TelemetryIngestService ingestService, JwtService jwtService) {
        this.ingestService = ingestService;
        this.jwtService = jwtService;
    }

    /**
     * Hodisalar paketini qabul qiladi.
     *
     * @param authentication the caller; the player id is taken from here, never from the body
     * @param batch          the events to ingest
     * @return {@code 202} with a per-batch summary
     */
    @PostMapping("/events")
    @Operation(
            summary = "Ingest a batch of telemetry events",
            description = """
                    Always returns 202, even when Kafka is unavailable: a lost event is a
                    small statistical gap, while a failed request is something the player
                    would actually notice. Telemetry never blocks the game.

                    The player id is taken from the access token; any playerId in the
                    body is ignored, so nobody can submit events as someone else.

                    Consent is enforced at ingest, not downstream. OFF produces nothing
                    at all; ANONYMOUS strips identifiers before the event leaves the
                    process.
                    """)
    @ApiResponses({
            @ApiResponse(responseCode = "202", description = "Batch accepted for asynchronous processing"),
            @ApiResponse(responseCode = "413", description = "Batch exceeds the configured maximum")
    })
    public ResponseEntity<TelemetryIngestService.IngestResult> ingest(
            Authentication authentication,
            @Valid @RequestBody TelemetryBatch batch) {

        UUID playerId = jwtService.requirePlayerId(authentication);
        return ResponseEntity.accepted().body(ingestService.ingest(playerId, batch.events()));
    }

    /**
     * A batch of events.
     *
     * <p>Chegara ikki joyda: bu yerdagi qat'iy yuqori chegara buzilgan klientdan
     * himoya qiladi, {@code ertugrul.telemetry.max-batch-size} esa ishlab
     * chiqarishda sozlanadigan haqiqiy limit.
     *
     * @param events the events; must be non-empty
     */
    public record TelemetryBatch(
            @NotEmpty(message = "batch must contain at least one event")
            @Size(max = 1000, message = "batch is too large")
            List<@Valid TelemetryEvent> events
    ) {
    }
}
