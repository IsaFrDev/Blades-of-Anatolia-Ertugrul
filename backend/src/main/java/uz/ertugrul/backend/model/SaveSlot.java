package uz.ertugrul.backend.model;

import uz.ertugrul.backend.json.Json;

/** Bulutdagi saqlash sloti — C++ klienti yuborgan snapshot. */
public record SaveSlot(String playerId, long updatedAt, int version, Json snapshot) {

    public Json toJson() {
        return Json.object()
                .set("playerId", playerId)
                .set("updatedAt", updatedAt)
                .set("version", version)
                .set("snapshot", snapshot);
    }

    public static SaveSlot fromJson(Json json) {
        return new SaveSlot(
                json.get("playerId").asString(),
                json.get("updatedAt").asLong(0),
                json.get("version").asInt(1),
                json.get("snapshot"));
    }

    public int honor() {
        return snapshot.get("gameState").get("honor").asInt(0);
    }

    public int episode() {
        return snapshot.get("gameState").get("episode").asInt(0);
    }

    public String quest() {
        return snapshot.get("gameState").get("quest").asString("");
    }
}
