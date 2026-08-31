package uz.ertugrul.backend.api;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;

import uz.ertugrul.backend.http.Http;
import uz.ertugrul.backend.json.Json;
import uz.ertugrul.backend.model.Player;
import uz.ertugrul.backend.model.SaveSlot;
import uz.ertugrul.backend.model.TelemetryEvent;
import uz.ertugrul.backend.service.ContentService;
import uz.ertugrul.backend.service.GameService;

/** REST endpointlari: o'yinchilar, saqlash, telemetriya, kontent, analitika. */
public final class ApiRoutes {

    private final GameService game;
    private final ContentService content;

    public ApiRoutes(GameService game, ContentService content) {
        this.game = game;
        this.content = content;
    }

    public void register(HttpServer server) {
        server.createContext("/api/health", this::health);
        server.createContext("/api/players", this::players);
        server.createContext("/api/saves", this::saves);
        server.createContext("/api/telemetry", this::telemetry);
        server.createContext("/api/content", this::contentRoutes);
        server.createContext("/api/analytics/overview", this::analytics);
    }

    // ------------------------------------------------------------ health

    private void health(HttpExchange exchange) throws IOException {
        if (Http.isPreflight(exchange)) {
            Http.ok(exchange, Json.object());
            return;
        }
        Http.ok(exchange, Json.object()
                .set("status", "ok")
                .set("service", "ertugrul-backend")
                .set("version", "0.1.0")
                .set("content", content.available()));
    }

    // ----------------------------------------------------------- players

    private void players(HttpExchange exchange) throws IOException {
        if (Http.isPreflight(exchange)) {
            Http.ok(exchange, Json.object());
            return;
        }
        final String method = exchange.getRequestMethod();
        final String id = Http.pathParam(exchange, "/api/players/");

        if ("POST".equals(method)) {
            final Json body = Http.readJson(exchange);
            final String playerId = body.get("playerId").asString();
            if (playerId.isEmpty()) {
                Http.error(exchange, 400, "playerId talab qilinadi");
                return;
            }
            final Player player = game.registerOrTouch(playerId, body.get("displayName").asString("Alp"));
            Http.json(exchange, 201, player.toJson());
            return;
        }

        if ("GET".equals(method)) {
            if (!id.isEmpty()) {
                final Player player = game.player(id);
                if (player == null) {
                    Http.error(exchange, 404, "o'yinchi topilmadi: " + id);
                    return;
                }
                Http.ok(exchange, player.toJson());
                return;
            }
            Json items = Json.array();
            for (Player player : game.players()) {
                items.add(player.toJson());
            }
            Http.ok(exchange, Json.object().set("count", items.size()).set("items", items));
            return;
        }
        Http.error(exchange, 405, "usul qo'llab-quvvatlanmaydi");
    }

    // ------------------------------------------------------------- saves

    private void saves(HttpExchange exchange) throws IOException {
        if (Http.isPreflight(exchange)) {
            Http.ok(exchange, Json.object());
            return;
        }
        final String method = exchange.getRequestMethod();
        final String id = Http.pathParam(exchange, "/api/saves/");

        switch (method) {
            case "POST" -> {
                final Json body = Http.readJson(exchange);
                final String playerId = body.get("playerId").asString();
                if (playerId.isEmpty()) {
                    Http.error(exchange, 400, "playerId talab qilinadi");
                    return;
                }
                game.registerOrTouch(playerId, body.get("displayName").asString("Alp"));
                final SaveSlot slot = game.putSave(playerId, body.get("snapshot"));
                game.recordEvent(playerId, "save", Json.object().set("version", slot.version()));
                Http.json(exchange, 201, Json.object()
                        .set("playerId", slot.playerId())
                        .set("version", slot.version())
                        .set("updatedAt", slot.updatedAt()));
            }
            case "GET" -> {
                if (id.isEmpty()) {
                    Http.error(exchange, 400, "playerId ko'rsatilmadi");
                    return;
                }
                final SaveSlot slot = game.getSave(id);
                if (slot == null) {
                    Http.error(exchange, 404, "saqlash topilmadi: " + id);
                    return;
                }
                Http.ok(exchange, slot.toJson());
            }
            case "DELETE" -> Http.ok(exchange, Json.object().set("deleted", game.deleteSave(id)));
            default -> Http.error(exchange, 405, "usul qo'llab-quvvatlanmaydi");
        }
    }

    // --------------------------------------------------------- telemetry

    private void telemetry(HttpExchange exchange) throws IOException {
        if (Http.isPreflight(exchange)) {
            Http.ok(exchange, Json.object());
            return;
        }
        if ("POST".equals(exchange.getRequestMethod())) {
            final Json body = Http.readJson(exchange);
            final TelemetryEvent event = game.recordEvent(
                    body.get("playerId").asString("anon"),
                    body.get("type").asString("unknown"),
                    body.get("payload"));
            Http.json(exchange, 201, Json.object().set("id", event.id()));
            return;
        }
        if ("GET".equals(exchange.getRequestMethod())) {
            final int limit = Integer.parseInt(Http.query(exchange).getOrDefault("limit", "100"));
            Json items = Json.array();
            for (TelemetryEvent event : game.events(limit)) {
                items.add(event.toJson());
            }
            Http.ok(exchange, Json.object().set("count", items.size()).set("items", items));
            return;
        }
        Http.error(exchange, 405, "usul qo'llab-quvvatlanmaydi");
    }

    // ----------------------------------------------------------- content

    private void contentRoutes(HttpExchange exchange) throws IOException {
        if (Http.isPreflight(exchange)) {
            Http.ok(exchange, Json.object());
            return;
        }
        final String path = exchange.getRequestURI().getPath();
        if (path.startsWith("/api/content/quests/")) {
            final String questId = Http.pathParam(exchange, "/api/content/quests/");
            final Json quest = content.quest(questId);
            if (quest.isNull()) {
                Http.error(exchange, 404, "kvest topilmadi: " + questId);
                return;
            }
            Http.ok(exchange, quest);
            return;
        }
        switch (path) {
            case "/api/content/quests" -> Http.ok(exchange, content.quests());
            case "/api/content/enemies" -> Http.ok(exchange, content.enemies());
            case "/api/content/cast" -> Http.ok(exchange, content.cast());
            case "/api/content/episodes" -> Http.ok(exchange, content.episodes());
            case "/api/content/validate" -> Http.ok(exchange, content.validate());
            default -> Http.error(exchange, 404, "noma'lum kontent yo'li");
        }
    }

    // --------------------------------------------------------- analytics

    private void analytics(HttpExchange exchange) throws IOException {
        if (Http.isPreflight(exchange)) {
            Http.ok(exchange, Json.object());
            return;
        }
        Http.ok(exchange, game.overview());
    }
}
