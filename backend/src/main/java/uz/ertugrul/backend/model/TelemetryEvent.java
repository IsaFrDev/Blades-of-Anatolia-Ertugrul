package uz.ertugrul.backend.model;

import uz.ertugrul.backend.json.Json;

/** Telemetriya hodisasi: maqsad bajarildi, o'lim, fosh bo'lish, boss fazasi va h.k. */
public record TelemetryEvent(long id, String playerId, String type, long timestamp, Json payload) {

    public Json toJson() {
        return Json.object()
                .set("id", id)
                .set("playerId", playerId)
                .set("type", type)
                .set("timestamp", timestamp)
                .set("payload", payload);
    }

    public static TelemetryEvent fromJson(Json json) {
        return new TelemetryEvent(
                json.get("id").asLong(0),
                json.get("playerId").asString(),
                json.get("type").asString("unknown"),
                json.get("timestamp").asLong(0),
                json.get("payload"));
    }
}
