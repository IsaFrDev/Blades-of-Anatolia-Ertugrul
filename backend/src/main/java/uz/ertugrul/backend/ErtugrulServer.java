package uz.ertugrul.backend;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.Executors;

import uz.ertugrul.backend.api.ApiRoutes;
import uz.ertugrul.backend.http.Http;
import uz.ertugrul.backend.json.Json;
import uz.ertugrul.backend.repo.JsonStore;
import uz.ertugrul.backend.service.ContentService;
import uz.ertugrul.backend.service.GameService;

/**
 * Dirilis: Ertugrul — backend.
 *
 * Faqat JDK 21 (tashqi bog'liqliksiz): com.sun.net.httpserver.
 * Ishga tushirish:  java -cp out uz.ertugrul.backend.ErtugrulServer [--port 8080] [--data ../data]
 *
 * Qatlamlar:  api (HTTP)  ->  service (mantiq)  ->  repo (fayl ombori)
 * Bu bo'linish tufayli keyinchalik Spring Boot yoki PostgreSQL ga o'tish oson.
 */
public final class ErtugrulServer {

    private final int port;
    private final Path dataRoot;
    private final Path storeRoot;
    private HttpServer server;

    public ErtugrulServer(int port, Path dataRoot, Path storeRoot) {
        this.port = port;
        this.dataRoot = dataRoot;
        this.storeRoot = storeRoot;
    }

    public void start() throws IOException {
        final JsonStore store = new JsonStore(storeRoot);
        final GameService game = new GameService(store);
        final ContentService content = new ContentService(dataRoot);

        server = HttpServer.create(new InetSocketAddress(port), 0);
        server.setExecutor(Executors.newVirtualThreadPerTaskExecutor());   // Java 21 virtual threads

        new ApiRoutes(game, content).register(server);
        server.createContext("/", this::staticFiles);

        server.start();
        System.out.println("""
                ===========================================
                 Dirilis: Ertugrul — backend ishga tushdi
                ===========================================
                 API:        http://localhost:%d/api/health
                 Dashboard:  http://localhost:%d/
                 Kontent:    %s (%s)
                 Ombor:      %s
                 To'xtatish: Ctrl+C
                """.formatted(port, port, dataRoot.toAbsolutePath(),
                              content.available() ? "topildi" : "TOPILMADI", storeRoot.toAbsolutePath()));
    }

    public void stop() {
        if (server != null) {
            server.stop(0);
        }
    }

    // --------------------------------------------------- statik fayllar

    /** Admin dashboard: resources/web/ ichidagi fayllar. */
    private void staticFiles(HttpExchange exchange) throws IOException {
        String path = exchange.getRequestURI().getPath();
        if (path.equals("/") || path.isEmpty()) {
            path = "/index.html";
        }
        if (path.contains("..")) {
            Http.error(exchange, 400, "noto'g'ri yo'l");
            return;
        }

        final String resource = "/web" + path;
        try (InputStream in = ErtugrulServer.class.getResourceAsStream(resource)) {
            if (in == null) {
                // Klasspatda topilmasa — manba papkasidan (ishlab chiqish rejimi)
                final Path fallback = Path.of("backend/src/main/resources/web").resolve(path.substring(1));
                if (Files.exists(fallback)) {
                    Http.send(exchange, 200, contentType(path), Files.readAllBytes(fallback));
                    return;
                }
                Http.text(exchange, 404, "text/plain; charset=utf-8", "Topilmadi: " + path);
                return;
            }
            Http.send(exchange, 200, contentType(path), in.readAllBytes());
        }
    }

    private static String contentType(String path) {
        if (path.endsWith(".html")) {
            return "text/html; charset=utf-8";
        }
        if (path.endsWith(".css")) {
            return "text/css; charset=utf-8";
        }
        if (path.endsWith(".js")) {
            return "application/javascript; charset=utf-8";
        }
        if (path.endsWith(".json")) {
            return "application/json; charset=utf-8";
        }
        if (path.endsWith(".svg")) {
            return "image/svg+xml";
        }
        return "application/octet-stream";
    }

    // ---------------------------------------------------------------- main

    public static void main(String[] args) throws IOException {
        int port = 8080;
        Path dataRoot = Path.of("data");
        Path storeRoot = Path.of("backend/data");

        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "--port" -> port = Integer.parseInt(args[++i]);
                case "--data" -> dataRoot = Path.of(args[++i]);
                case "--store" -> storeRoot = Path.of(args[++i]);
                case "--help" -> {
                    System.out.println("""
                            Dirilis: Ertugrul backend
                              --port <n>     HTTP porti (default 8080)
                              --data <yo'l>  o'yin kontenti papkasi (default data)
                              --store <yo'l> ombor papkasi (default backend/data)
                            """);
                    return;
                }
                default -> System.err.println("Noma'lum argument: " + args[i]);
            }
        }

        final ErtugrulServer server = new ErtugrulServer(port, dataRoot, storeRoot);
        server.start();

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("Backend to'xtatilmoqda...");
            server.stop();
        }));
    }

    /** Sinov uchun qulaylik: server holatini JSON ko'rinishida qaytaradi. */
    public Json describe() {
        return Json.object()
                .set("port", port)
                .set("data", dataRoot.toString())
                .set("store", storeRoot.toString())
                .set("charset", StandardCharsets.UTF_8.name());
    }
}
