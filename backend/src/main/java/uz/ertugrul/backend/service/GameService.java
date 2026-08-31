package uz.ertugrul.backend.service;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;

import uz.ertugrul.backend.json.Json;
import uz.ertugrul.backend.model.Player;
import uz.ertugrul.backend.model.SaveSlot;
import uz.ertugrul.backend.model.TelemetryEvent;
import uz.ertugrul.backend.repo.JsonStore;

/**
 * Biznes mantiq: o'yinchilar, bulut saqlash, telemetriya va analitika.
 * (Kontent xizmati alohida — ContentService.)
 */
public class GameService {

    private static final String PLAYERS = "players";
    private static final String SAVES = "saves";
    private static final String TELEMETRY_LOG = "telemetry";

    private final JsonStore store;
    private final AtomicLong eventCounter = new AtomicLong();

    public GameService(JsonStore store) {
        this.store = store;
        this.eventCounter.set(store.logSize(TELEMETRY_LOG));
    }

    private static long now() {
        return Instant.now().toEpochMilli();
    }

    // ---------------------------------------------------------- o'yinchilar

    public Player registerOrTouch(String playerId, String displayName) {
        final Json existing = store.read(PLAYERS, playerId);
        Player player;
        if (existing.isNull()) {
            player = Player.of(playerId, displayName == null || displayName.isBlank() ? "Alp" : displayName, now());
        } else {
            final Player previous = Player.fromJson(existing);
            player = previous.withProgress(previous.honor(), previous.episode(), previous.currentQuest(), now(), 0);
        }
        store.write(PLAYERS, playerId, player.toJson());
        return player;
    }

    public Player player(String playerId) {
        final Json json = store.read(PLAYERS, playerId);
        return json.isNull() ? null : Player.fromJson(json);
    }

    public List<Player> players() {
        List<Player> result = new ArrayList<>();
        for (Json json : store.readAll(PLAYERS)) {
            result.add(Player.fromJson(json));
        }
        result.sort(Comparator.comparingLong(Player::lastSeenAt).reversed());
        return result;
    }

    // -------------------------------------------------------- bulut saqlash

    public SaveSlot putSave(String playerId, Json snapshot) {
        final Json previous = store.read(SAVES, playerId);
        final int version = previous.isNull() ? 1 : previous.get("version").asInt(1) + 1;
        final SaveSlot slot = new SaveSlot(playerId, now(), version, snapshot);
        store.write(SAVES, playerId, slot.toJson());

        // O'yinchi progressini saqlashdan yangilaymiz
        final Player player = player(playerId);
        if (player != null) {
            store.write(PLAYERS, playerId,
                    player.withProgress(slot.honor(), slot.episode(), slot.quest(), now(), 0).toJson());
        }
        return slot;
    }

    public SaveSlot getSave(String playerId) {
        final Json json = store.read(SAVES, playerId);
        return json.isNull() ? null : SaveSlot.fromJson(json);
    }

    public boolean deleteSave(String playerId) {
        return store.delete(SAVES, playerId);
    }

    // ---------------------------------------------------------- telemetriya

    public TelemetryEvent recordEvent(String playerId, String type, Json payload) {
        final TelemetryEvent event = new TelemetryEvent(
                eventCounter.incrementAndGet(), playerId, type, now(), payload);
        store.append(TELEMETRY_LOG, event.toJson());
        return event;
    }

    public List<TelemetryEvent> events(int limit) {
        List<TelemetryEvent> result = new ArrayList<>();
        for (Json json : store.readLog(TELEMETRY_LOG, limit)) {
            result.add(TelemetryEvent.fromJson(json));
        }
        return result;
    }

    // ------------------------------------------------------------ analitika

    /**
     * Dashboard uchun umumiy ko'rsatkichlar: o'yinchilar, epizodlar bo'yicha taqsimot,
     * eng ko'p bajarilgan maqsadlar, o'lim/fosh bo'lish nisbati.
     */
    public Json overview() {
        final List<Player> players = players();
        final List<TelemetryEvent> events = events(5000);

        int totalHonor = 0;
        Map<String, Integer> episodeCounts = new LinkedHashMap<>();
        for (Player player : players) {
            totalHonor += player.honor();
            episodeCounts.merge("Epizod " + (player.episode() + 1), 1, Integer::sum);
        }

        Map<String, Integer> eventCounts = new LinkedHashMap<>();
        Map<String, Integer> objectiveCounts = new LinkedHashMap<>();
        for (TelemetryEvent event : events) {
            eventCounts.merge(event.type(), 1, Integer::sum);
            if ("objective".equals(event.type())) {
                final String key = event.payload().get("quest").asString("?")
                        + " / " + event.payload().get("objective").asString("?");
                objectiveCounts.merge(key, 1, Integer::sum);
            }
        }

        Json json = Json.object();
        json.set("players", players.size());
        json.set("events", events.size());
        json.set("saves", store.readAll(SAVES).size());
        json.set("averageHonor", players.isEmpty() ? 0 : totalHonor / players.size());
        json.set("episodes", mapToJson(episodeCounts));
        json.set("eventTypes", mapToJson(eventCounts));
        json.set("topObjectives", topEntries(objectiveCounts, 8));
        json.set("generatedAt", now());
        return json;
    }

    private static Json mapToJson(Map<String, Integer> map) {
        Json json = Json.array();
        map.forEach((key, value) -> json.add(Json.object().set("label", key).set("value", value)));
        return json;
    }

    private static Json topEntries(Map<String, Integer> map, int limit) {
        return map.entrySet().stream()
                .sorted(Map.Entry.<String, Integer>comparingByValue().reversed())
                .limit(limit)
                .collect(Json::array,
                        (json, entry) -> json.add(Json.object()
                                .set("label", entry.getKey())
                                .set("value", entry.getValue())),
                        (a, b) -> b.items().forEach(a::add));
    }
}
