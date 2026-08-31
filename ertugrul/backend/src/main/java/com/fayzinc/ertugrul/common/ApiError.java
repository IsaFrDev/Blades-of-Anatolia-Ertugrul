package com.fayzinc.ertugrul.common;

import com.fasterxml.jackson.annotation.JsonInclude;
import io.swagger.v3.oas.annotations.media.Schema;

import java.time.Instant;
import java.util.List;
import java.util.Map;

/**
 * Uniform error envelope for every non-2xx response.
 *
 * <p>Shakl ataylab sodda: UE5 klienti JSON'ni {@code FErtApiError} struct'iga
 * deserializatsiya qiladi, shuning uchun maydonlar soni barqaror bo'lishi kerak.
 * {@code code} — klient shu bo'yicha qaror qabul qiladi, {@code message} esa
 * faqat log va QA uchun (o'yinchiga hech qachon ko'rsatilmaydi — u lokalizatsiya
 * qilinmagan).
 *
 * @param code      stable machine-readable error code
 * @param message   developer-facing description; never shown to the player
 * @param path      request path that produced the error
 * @param traceId   correlation id for log lookup
 * @param timestamp server time
 * @param details   optional field-level validation failures
 */
@JsonInclude(JsonInclude.Include.NON_NULL)
@Schema(name = "ApiError", description = "Uniform error envelope")
public record ApiError(

        @Schema(example = "SAVE_CONFLICT")
        String code,

        @Schema(example = "Vector clocks are concurrent for slot 3")
        String message,

        @Schema(example = "/api/v1/saves/3")
        String path,

        @Schema(example = "c0ffee1234abcd")
        String traceId,

        Instant timestamp,

        @Schema(description = "Field-level validation failures, when applicable")
        List<FieldViolation> details
) {

    /**
     * One rejected field from bean validation.
     *
     * @param field   dotted path of the offending property
     * @param reason  human-readable constraint message
     * @param rejected the value that failed, stringified and truncated
     */
    @JsonInclude(JsonInclude.Include.NON_NULL)
    public record FieldViolation(String field, String reason, String rejected) {
    }

    public static ApiError of(ErtugrulException.Code code, String message, String path, String traceId) {
        return new ApiError(code.name(), message, path, traceId, Instant.now(), null);
    }

    public static ApiError validation(String path, String traceId, List<FieldViolation> details) {
        return new ApiError(
                ErtugrulException.Code.VALIDATION_FAILED.name(),
                "Request validation failed",
                path, traceId, Instant.now(), details);
    }

    /**
     * Conflict responses carry extra context so the client can render the
     * "which save do you want to keep?" dialog without a second round trip.
     */
    public ApiError withDetails(List<FieldViolation> extra) {
        return new ApiError(code, message, path, traceId, timestamp, extra);
    }

    public static List<FieldViolation> fromMap(Map<String, String> fieldErrors) {
        return fieldErrors.entrySet().stream()
                .map(e -> new FieldViolation(e.getKey(), e.getValue(), null))
                .toList();
    }
}
