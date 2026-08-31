package uz.ertugrul.backend.service;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

import uz.ertugrul.backend.json.Json;

/**
 * Kontent xizmati: kvestlar, dushmanlar, kast va epizodlar — o'yin bilan bir xil data/ papkasidan.
 * Shu tufayli klient va backend bitta manbadan foydalanadi (kontentni jonli yangilash mumkin).
 */
public class ContentService {

    private final Path dataRoot;

    public ContentService(Path dataRoot) {
        this.dataRoot = dataRoot;
    }

    public boolean available() {
        return Files.isDirectory(dataRoot);
    }

    public Json quests() {
        Json array = Json.array();
        for (Json quest : readDirectory(dataRoot.resolve("quests"))) {
            array.add(quest);
        }
        return Json.object().set("count", array.size()).set("items", array);
    }

    public Json quest(String questId) {
        for (Json quest : readDirectory(dataRoot.resolve("quests"))) {
            if (quest.get("id").asString().equals(questId)) {
                return quest;
            }
        }
        return Json.ofNull();
    }

    public Json enemies() {
        Json merged = Json.object();
        for (Json file : readDirectory(dataRoot.resolve("enemies"))) {
            file.fields().forEach(merged::set);
        }
        return merged;
    }

    public Json cast() {
        return readFile(dataRoot.resolve("characters").resolve("cast.json"));
    }

    public Json episodes() {
        return readFile(dataRoot.resolve("episodes").resolve("episodes.json"));
    }

    /** Kvest zanjirini tekshirish: "next" havolalari mavjud kvestlarga olib boradimi? */
    public Json validate() {
        final List<Json> quests = readDirectory(dataRoot.resolve("quests"));
        List<String> ids = new ArrayList<>();
        for (Json quest : quests) {
            ids.add(quest.get("id").asString());
        }

        Json problems = Json.array();
        for (Json quest : quests) {
            final String next = quest.get("next").asString("");
            if (!next.isEmpty() && !ids.contains(next)) {
                problems.add(Json.object()
                        .set("quest", quest.get("id").asString())
                        .set("problem", "noma'lum keyingi kvest: " + next));
            }
            if (quest.get("objectives").size() == 0) {
                problems.add(Json.object()
                        .set("quest", quest.get("id").asString())
                        .set("problem", "maqsadlar yo'q"));
            }
        }
        return Json.object()
                .set("quests", quests.size())
                .set("problems", problems)
                .set("ok", problems.size() == 0);
    }

    private List<Json> readDirectory(Path directory) {
        List<Json> result = new ArrayList<>();
        if (!Files.isDirectory(directory)) {
            return result;
        }
        try (var stream = Files.list(directory)) {
            stream.filter(path -> path.toString().endsWith(".json"))
                  .sorted()
                  .forEach(path -> result.add(readFile(path)));
        } catch (IOException ignored) {
            // papka o'qilmadi
        }
        return result;
    }

    private Json readFile(Path path) {
        try {
            return Json.parse(Files.readString(path, StandardCharsets.UTF_8));
        } catch (IOException e) {
            return Json.ofNull();
        }
    }
}
