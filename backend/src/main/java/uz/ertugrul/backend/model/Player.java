package uz.ertugrul.backend.model;

import uz.ertugrul.backend.json.Json;

/** O'yinchi profili. */
public record Player(
        String playerId,
        String displayName,
        int honor,
        int episode,
        String currentQuest,
        long createdAt,
        long lastSeenAt,
        int playSeconds
) {
    public static Player of(String playerId, String displayName, long now) {
        return new Player(playerId, displayName, 0, 0, "", now, now, 0);
    }

    public Player withProgress(int honor, int episode, String quest, long now, int extraSeconds) {
        return new Player(playerId, displayName, honor, episode, quest, createdAt, now, playSeconds + extraSeconds);
    }

    public Json toJson() {
        return Json.object()
                .set("playerId", playerId)
                .set("displayName", displayName)
                .set("honor", honor)
                .set("episode", episode)
                .set("currentQuest", currentQuest)
                .set("createdAt", createdAt)
                .set("lastSeenAt", lastSeenAt)
                .set("playSeconds", playSeconds);
    }

    public static Player fromJson(Json json) {
        return new Player(
                json.get("playerId").asString(),
                json.get("displayName").asString("Alp"),
                json.get("honor").asInt(0),
                json.get("episode").asInt(0),
                json.get("currentQuest").asString(""),
                json.get("createdAt").asLong(0),
                json.get("lastSeenAt").asLong(0),
                json.get("playSeconds").asInt(0));
    }
}
