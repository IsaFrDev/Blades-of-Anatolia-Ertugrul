package uz.ertugrul.backend.json;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Minimal JSON qiymat + parser/serializer.
 * Tashqi kutubxonasiz (loyiha faqat JDK 21 ga tayanadi).
 */
public final class Json {

    public enum Type { NULL, BOOL, NUMBER, STRING, ARRAY, OBJECT }

    private Type type = Type.NULL;
    private boolean boolValue;
    private double numberValue;
    private String stringValue;
    private List<Json> array;
    private Map<String, Json> object;

    // ------------------------------------------------------------ yaratish

    public static Json ofNull() {
        return new Json();
    }

    public static Json of(boolean value) {
        Json json = new Json();
        json.type = Type.BOOL;
        json.boolValue = value;
        return json;
    }

    public static Json of(double value) {
        Json json = new Json();
        json.type = Type.NUMBER;
        json.numberValue = value;
        return json;
    }

    public static Json of(long value) {
        return of((double) value);
    }

    public static Json of(String value) {
        if (value == null) {
            return ofNull();
        }
        Json json = new Json();
        json.type = Type.STRING;
        json.stringValue = value;
        return json;
    }

    public static Json array() {
        Json json = new Json();
        json.type = Type.ARRAY;
        json.array = new ArrayList<>();
        return json;
    }

    public static Json object() {
        Json json = new Json();
        json.type = Type.OBJECT;
        json.object = new LinkedHashMap<>();
        return json;
    }

    // -------------------------------------------------------------- o'qish

    public Type type() {
        return type;
    }

    public boolean isNull() {
        return type == Type.NULL;
    }

    public boolean isObject() {
        return type == Type.OBJECT;
    }

    public boolean isArray() {
        return type == Type.ARRAY;
    }

    public boolean asBool(boolean fallback) {
        return switch (type) {
            case BOOL -> boolValue;
            case NUMBER -> numberValue != 0;
            default -> fallback;
        };
    }

    public double asDouble(double fallback) {
        return switch (type) {
            case NUMBER -> numberValue;
            case BOOL -> boolValue ? 1 : 0;
            case STRING -> {
                try {
                    yield Double.parseDouble(stringValue);
                } catch (NumberFormatException e) {
                    yield fallback;
                }
            }
            default -> fallback;
        };
    }

    public int asInt(int fallback) {
        return (int) asDouble(fallback);
    }

    public long asLong(long fallback) {
        return (long) asDouble(fallback);
    }

    public String asString(String fallback) {
        return switch (type) {
            case STRING -> stringValue;
            case NUMBER -> numberValue == Math.rint(numberValue)
                    ? String.valueOf((long) numberValue) : String.valueOf(numberValue);
            case BOOL -> String.valueOf(boolValue);
            default -> fallback;
        };
    }

    public String asString() {
        return asString("");
    }

    public Json get(String key) {
        if (type != Type.OBJECT) {
            return ofNull();
        }
        return object.getOrDefault(key, ofNull());
    }

    public Json get(int index) {
        if (type != Type.ARRAY || index < 0 || index >= array.size()) {
            return ofNull();
        }
        return array.get(index);
    }

    public boolean has(String key) {
        return type == Type.OBJECT && object.containsKey(key);
    }

    public int size() {
        return switch (type) {
            case ARRAY -> array.size();
            case OBJECT -> object.size();
            default -> 0;
        };
    }

    public List<Json> items() {
        return type == Type.ARRAY ? array : List.of();
    }

    public Map<String, Json> fields() {
        return type == Type.OBJECT ? object : Map.of();
    }

    // -------------------------------------------------------------- yozish

    public Json set(String key, Json value) {
        if (type != Type.OBJECT) {
            type = Type.OBJECT;
            object = new LinkedHashMap<>();
        }
        object.put(key, value == null ? ofNull() : value);
        return this;
    }

    public Json set(String key, String value) {
        return set(key, of(value));
    }

    public Json set(String key, double value) {
        return set(key, of(value));
    }

    public Json set(String key, long value) {
        return set(key, of(value));
    }

    public Json set(String key, boolean value) {
        return set(key, of(value));
    }

    public Json add(Json value) {
        if (type != Type.ARRAY) {
            type = Type.ARRAY;
            array = new ArrayList<>();
        }
        array.add(value == null ? ofNull() : value);
        return this;
    }

    // ---------------------------------------------------------- serializer

    @Override
    public String toString() {
        StringBuilder out = new StringBuilder();
        write(out, -1, 0);
        return out.toString();
    }

    public String toPrettyString() {
        StringBuilder out = new StringBuilder();
        write(out, 2, 0);
        return out.toString();
    }

    private void write(StringBuilder out, int indent, int depth) {
        final boolean pretty = indent >= 0;
        final String pad = pretty ? " ".repeat(indent * (depth + 1)) : "";
        final String padEnd = pretty ? " ".repeat(indent * depth) : "";
        final String newline = pretty ? "\n" : "";

        switch (type) {
            case NULL -> out.append("null");
            case BOOL -> out.append(boolValue);
            case NUMBER -> {
                if (numberValue == Math.rint(numberValue) && !Double.isInfinite(numberValue)) {
                    out.append((long) numberValue);
                } else {
                    out.append(numberValue);
                }
            }
            case STRING -> escape(out, stringValue);
            case ARRAY -> {
                if (array.isEmpty()) {
                    out.append("[]");
                    return;
                }
                out.append('[').append(newline);
                for (int i = 0; i < array.size(); i++) {
                    out.append(pad);
                    array.get(i).write(out, indent, depth + 1);
                    if (i + 1 < array.size()) {
                        out.append(',');
                    }
                    out.append(newline);
                }
                out.append(padEnd).append(']');
            }
            case OBJECT -> {
                if (object.isEmpty()) {
                    out.append("{}");
                    return;
                }
                out.append('{').append(newline);
                int i = 0;
                for (Map.Entry<String, Json> entry : object.entrySet()) {
                    out.append(pad);
                    escape(out, entry.getKey());
                    out.append(pretty ? ": " : ":");
                    entry.getValue().write(out, indent, depth + 1);
                    if (++i < object.size()) {
                        out.append(',');
                    }
                    out.append(newline);
                }
                out.append(padEnd).append('}');
            }
        }
    }

    private static void escape(StringBuilder out, String value) {
        out.append('"');
        for (int i = 0; i < value.length(); i++) {
            final char c = value.charAt(i);
            switch (c) {
                case '"' -> out.append("\\\"");
                case '\\' -> out.append("\\\\");
                case '\n' -> out.append("\\n");
                case '\r' -> out.append("\\r");
                case '\t' -> out.append("\\t");
                default -> {
                    if (c < 0x20) {
                        out.append(String.format("\\u%04x", (int) c));
                    } else {
                        out.append(c);
                    }
                }
            }
        }
        out.append('"');
    }

    // ------------------------------------------------------------- parser

    public static Json parse(String text) {
        if (text == null || text.isBlank()) {
            return ofNull();
        }
        return new Parser(text).parseValue();
    }

    private static final class Parser {
        private final String text;
        private int pos;

        Parser(String text) {
            this.text = text;
        }

        Json parseValue() {
            skipWhitespace();
            if (pos >= text.length()) {
                return ofNull();
            }
            return switch (text.charAt(pos)) {
                case '{' -> parseObject();
                case '[' -> parseArray();
                case '"' -> of(parseString());
                case 't', 'f' -> parseBool();
                case 'n' -> {
                    pos += 4;
                    yield ofNull();
                }
                default -> parseNumber();
            };
        }

        private void skipWhitespace() {
            while (pos < text.length() && Character.isWhitespace(text.charAt(pos))) {
                pos++;
            }
        }

        private boolean consume(char expected) {
            skipWhitespace();
            if (pos < text.length() && text.charAt(pos) == expected) {
                pos++;
                return true;
            }
            return false;
        }

        private Json parseObject() {
            Json result = object();
            pos++;
            if (consume('}')) {
                return result;
            }
            while (pos < text.length()) {
                skipWhitespace();
                if (pos >= text.length() || text.charAt(pos) != '"') {
                    return result;
                }
                final String key = parseString();
                if (!consume(':')) {
                    return result;
                }
                result.set(key, parseValue());
                if (consume(',')) {
                    continue;
                }
                consume('}');
                return result;
            }
            return result;
        }

        private Json parseArray() {
            Json result = array();
            pos++;
            if (consume(']')) {
                return result;
            }
            while (pos < text.length()) {
                result.add(parseValue());
                if (consume(',')) {
                    continue;
                }
                consume(']');
                return result;
            }
            return result;
        }

        private Json parseBool() {
            if (text.startsWith("true", pos)) {
                pos += 4;
                return of(true);
            }
            if (text.startsWith("false", pos)) {
                pos += 5;
                return of(false);
            }
            pos++;
            return ofNull();
        }

        private Json parseNumber() {
            final int start = pos;
            while (pos < text.length()) {
                final char c = text.charAt(pos);
                if (Character.isDigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
                    pos++;
                } else {
                    break;
                }
            }
            if (start == pos) {
                pos++;
                return ofNull();
            }
            try {
                return of(Double.parseDouble(text.substring(start, pos)));
            } catch (NumberFormatException e) {
                return ofNull();
            }
        }

        private String parseString() {
            StringBuilder out = new StringBuilder();
            pos++;
            while (pos < text.length() && text.charAt(pos) != '"') {
                char c = text.charAt(pos);
                if (c == '\\' && pos + 1 < text.length()) {
                    pos++;
                    switch (text.charAt(pos)) {
                        case 'n' -> out.append('\n');
                        case 't' -> out.append('\t');
                        case 'r' -> out.append('\r');
                        case 'b' -> out.append('\b');
                        case 'f' -> out.append('\f');
                        case '"' -> out.append('"');
                        case '\\' -> out.append('\\');
                        case '/' -> out.append('/');
                        case 'u' -> {
                            if (pos + 4 < text.length()) {
                                out.append((char) Integer.parseInt(text.substring(pos + 1, pos + 5), 16));
                                pos += 4;
                            }
                        }
                        default -> out.append(text.charAt(pos));
                    }
                } else {
                    out.append(c);
                }
                pos++;
            }
            pos++;
            return out.toString();
        }
    }
}
