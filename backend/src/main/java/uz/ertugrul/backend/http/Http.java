package uz.ertugrul.backend.http;

import com.sun.net.httpserver.HttpExchange;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

import uz.ertugrul.backend.json.Json;

/** HTTP yordamchilari: javob yozish, so'rov tanasini o'qish, query parslash. */
public final class Http {

    private Http() {
    }

    public static String readBody(HttpExchange exchange) throws IOException {
        try (InputStream in = exchange.getRequestBody()) {
            return new String(in.readAllBytes(), StandardCharsets.UTF_8);
        }
    }

    public static Json readJson(HttpExchange exchange) throws IOException {
        return Json.parse(readBody(exchange));
    }

    public static void json(HttpExchange exchange, int status, Json body) throws IOException {
        send(exchange, status, "application/json; charset=utf-8", body.toString().getBytes(StandardCharsets.UTF_8));
    }

    public static void ok(HttpExchange exchange, Json body) throws IOException {
        json(exchange, 200, body);
    }

    public static void error(HttpExchange exchange, int status, String message) throws IOException {
        json(exchange, status, Json.object().set("error", message).set("status", status));
    }

    public static void text(HttpExchange exchange, int status, String contentType, String body) throws IOException {
        send(exchange, status, contentType, body.getBytes(StandardCharsets.UTF_8));
    }

    public static void send(HttpExchange exchange, int status, String contentType, byte[] body) throws IOException {
        exchange.getResponseHeaders().add("Content-Type", contentType);
        exchange.getResponseHeaders().add("Cache-Control", "no-store");
        // Lokal o'yin klienti va dashboard uchun
        exchange.getResponseHeaders().add("Access-Control-Allow-Origin", "*");
        exchange.getResponseHeaders().add("Access-Control-Allow-Headers", "Content-Type, X-Player-Id");
        exchange.getResponseHeaders().add("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        exchange.sendResponseHeaders(status, body.length);
        try (OutputStream out = exchange.getResponseBody()) {
            out.write(body);
        }
    }

    public static Map<String, String> query(HttpExchange exchange) {
        Map<String, String> result = new HashMap<>();
        final String raw = exchange.getRequestURI().getRawQuery();
        if (raw == null || raw.isEmpty()) {
            return result;
        }
        for (String pair : raw.split("&")) {
            final int eq = pair.indexOf('=');
            if (eq <= 0) {
                continue;
            }
            result.put(URLDecoder.decode(pair.substring(0, eq), StandardCharsets.UTF_8),
                       URLDecoder.decode(pair.substring(eq + 1), StandardCharsets.UTF_8));
        }
        return result;
    }

    /** "/api/saves/ertugrul" + prefiks "/api/saves/" -> "ertugrul" */
    public static String pathParam(HttpExchange exchange, String prefix) {
        final String path = exchange.getRequestURI().getPath();
        if (!path.startsWith(prefix)) {
            return "";
        }
        final String rest = path.substring(prefix.length());
        final int slash = rest.indexOf('/');
        return slash < 0 ? rest : rest.substring(0, slash);
    }

    public static boolean isPreflight(HttpExchange exchange) {
        return "OPTIONS".equalsIgnoreCase(exchange.getRequestMethod());
    }
}
