package uz.ertugrul.backend.repo;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import uz.ertugrul.backend.json.Json;

/**
 * Oddiy fayl asosidagi JSON ombori (bitta jarayon uchun xavfsiz).
 * Keyinchalik PostgreSQL/Mongo repozitoriyasi shu interfeys o'rniga qo'yiladi.
 */
public class JsonStore {

    private final Path root;
    private final ReadWriteLock lock = new ReentrantReadWriteLock();

    public JsonStore(Path root) {
        this.root = root;
        try {
            Files.createDirectories(root);
        } catch (IOException e) {
            throw new IllegalStateException("Ombor papkasi yaratilmadi: " + root, e);
        }
    }

    public Path root() {
        return root;
    }

    /** Atomik yozish: avval .tmp ga, keyin almashtirish. */
    public void write(String collection, String id, Json value) {
        lock.writeLock().lock();
        try {
            final Path dir = root.resolve(collection);
            Files.createDirectories(dir);
            final Path target = dir.resolve(safe(id) + ".json");
            final Path temp = dir.resolve(safe(id) + ".json.tmp");
            Files.writeString(temp, value.toPrettyString(), StandardCharsets.UTF_8);
            Files.move(temp, target, StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            throw new IllegalStateException("Yozib bo'lmadi: " + collection + "/" + id, e);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public Json read(String collection, String id) {
        lock.readLock().lock();
        try {
            final Path path = root.resolve(collection).resolve(safe(id) + ".json");
            if (!Files.exists(path)) {
                return Json.ofNull();
            }
            return Json.parse(Files.readString(path, StandardCharsets.UTF_8));
        } catch (IOException e) {
            return Json.ofNull();
        } finally {
            lock.readLock().unlock();
        }
    }

    public boolean exists(String collection, String id) {
        return Files.exists(root.resolve(collection).resolve(safe(id) + ".json"));
    }

    public boolean delete(String collection, String id) {
        lock.writeLock().lock();
        try {
            return Files.deleteIfExists(root.resolve(collection).resolve(safe(id) + ".json"));
        } catch (IOException e) {
            return false;
        } finally {
            lock.writeLock().unlock();
        }
    }

    public List<Json> readAll(String collection) {
        lock.readLock().lock();
        List<Json> result = new ArrayList<>();
        try {
            final Path dir = root.resolve(collection);
            if (!Files.isDirectory(dir)) {
                return result;
            }
            try (var stream = Files.list(dir)) {
                stream.filter(path -> path.toString().endsWith(".json"))
                      .sorted()
                      .forEach(path -> {
                          try {
                              result.add(Json.parse(Files.readString(path, StandardCharsets.UTF_8)));
                          } catch (IOException ignored) {
                              // buzilgan fayl - o'tkazib yuboramiz
                          }
                      });
            }
        } catch (IOException ignored) {
            // papka yo'q
        } finally {
            lock.readLock().unlock();
        }
        return result;
    }

    /** Qo'shimchali jurnal (telemetriya uchun) — bitta JSONL fayl. */
    public void append(String logName, Json value) {
        lock.writeLock().lock();
        try {
            final Path path = root.resolve(logName + ".jsonl");
            Files.writeString(path, value + System.lineSeparator(), StandardCharsets.UTF_8,
                    java.nio.file.StandardOpenOption.CREATE, java.nio.file.StandardOpenOption.APPEND);
        } catch (IOException e) {
            throw new IllegalStateException("Jurnalga yozib bo'lmadi: " + logName, e);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public List<Json> readLog(String logName, int limit) {
        lock.readLock().lock();
        List<Json> result = new ArrayList<>();
        try {
            final Path path = root.resolve(logName + ".jsonl");
            if (!Files.exists(path)) {
                return result;
            }
            final List<String> lines = Files.readAllLines(path, StandardCharsets.UTF_8);
            final int from = limit > 0 ? Math.max(0, lines.size() - limit) : 0;
            for (int i = lines.size() - 1; i >= from; i--) {   // eng yangisi birinchi
                final String line = lines.get(i);
                if (!line.isBlank()) {
                    result.add(Json.parse(line));
                }
            }
        } catch (IOException ignored) {
            // o'qib bo'lmadi
        } finally {
            lock.readLock().unlock();
        }
        return result;
    }

    public int logSize(String logName) {
        try {
            final Path path = root.resolve(logName + ".jsonl");
            if (!Files.exists(path)) {
                return 0;
            }
            try (var stream = Files.lines(path, StandardCharsets.UTF_8)) {
                return (int) stream.filter(line -> !line.isBlank()).count();
            }
        } catch (IOException e) {
            return 0;
        }
    }

    private static String safe(String id) {
        return id.replaceAll("[^A-Za-z0-9_.-]", "_");
    }
}
