package com.fayzinc.ertugrul.common;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.ConstraintViolationException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.security.access.AccessDeniedException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;
import org.springframework.web.multipart.MaxUploadSizeExceededException;
import org.springframework.web.servlet.NoHandlerFoundException;

import java.util.List;
import java.util.Objects;

/**
 * Translates every thrown exception into an {@link ApiError}.
 *
 * <p>Ikki qat'iy qoida bor:
 * <ol>
 *   <li><b>Stack trace hech qachon javobga chiqmaydi.</b> Klient — shipped
 *       o'yin binary'si, u xato tafsilotini ko'rsatmaydi; tafsilot faqat
 *       serverda log qilinadi va {@code traceId} orqali bog'lanadi.</li>
 *   <li><b>Kutilgan xato WARN, kutilmagani ERROR.</b> Save konflikti — normal
 *       hodisa (ikki qurilma), uni ERROR qilib log qilsak, alert shovqinga
 *       ko'miladi va haqiqiy incident ko'rinmay qoladi.</li>
 * </ol>
 */
@RestControllerAdvice
public class GlobalExceptionHandler {

    private static final Logger log = LoggerFactory.getLogger(GlobalExceptionHandler.class);

    /** Value shown instead of a rejected field value that may be large or sensitive. */
    private static final int MAX_REJECTED_LEN = 120;

    @ExceptionHandler(ErtugrulException.class)
    public ResponseEntity<ApiError> handleDomain(ErtugrulException ex, HttpServletRequest req) {
        String traceId = traceId();
        HttpStatus status = ex.status();

        if (status.is5xxServerError()) {
            log.error("[{}] {} at {}", traceId, ex.code(), req.getRequestURI(), ex);
        } else {
            // Expected outcomes (stale save, conflict, rate limit) stay at WARN
            // so real incidents remain visible in the error rate.
            log.warn("[{}] {} at {}: {}", traceId, ex.code(), req.getRequestURI(), ex.getMessage());
        }

        ResponseEntity.BodyBuilder builder = ResponseEntity.status(status);
        if (ex.code() == ErtugrulException.Code.RATE_LIMITED) {
            // Tell the client when to come back instead of letting it hammer us.
            builder.header(HttpHeaders.RETRY_AFTER, "30");
        }
        return builder.body(ApiError.of(ex.code(), ex.getMessage(), req.getRequestURI(), traceId));
    }

    /** Bean validation on {@code @RequestBody} records. */
    @ExceptionHandler(MethodArgumentNotValidException.class)
    public ResponseEntity<ApiError> handleBodyValidation(MethodArgumentNotValidException ex,
                                                         HttpServletRequest req) {
        String traceId = traceId();
        List<ApiError.FieldViolation> details = ex.getBindingResult().getFieldErrors().stream()
                .map(fe -> new ApiError.FieldViolation(
                        fe.getField(),
                        Objects.requireNonNullElse(fe.getDefaultMessage(), "invalid"),
                        truncate(fe.getRejectedValue())))
                .toList();

        log.warn("[{}] validation failed at {}: {}", traceId, req.getRequestURI(), details);
        return ResponseEntity.badRequest()
                .body(ApiError.validation(req.getRequestURI(), traceId, details));
    }

    /** Bean validation on {@code @PathVariable} / {@code @RequestParam}. */
    @ExceptionHandler(ConstraintViolationException.class)
    public ResponseEntity<ApiError> handleParamValidation(ConstraintViolationException ex,
                                                          HttpServletRequest req) {
        String traceId = traceId();
        List<ApiError.FieldViolation> details = ex.getConstraintViolations().stream()
                .map(v -> new ApiError.FieldViolation(
                        v.getPropertyPath().toString(),
                        v.getMessage(),
                        truncate(v.getInvalidValue())))
                .toList();

        log.warn("[{}] param validation failed at {}: {}", traceId, req.getRequestURI(), details);
        return ResponseEntity.badRequest()
                .body(ApiError.validation(req.getRequestURI(), traceId, details));
    }

    /** Malformed JSON — usually a client serialisation bug worth surfacing. */
    @ExceptionHandler(HttpMessageNotReadableException.class)
    public ResponseEntity<ApiError> handleUnreadable(HttpMessageNotReadableException ex,
                                                     HttpServletRequest req) {
        String traceId = traceId();
        log.warn("[{}] unreadable body at {}: {}", traceId, req.getRequestURI(), ex.getMessage());
        return ResponseEntity.badRequest().body(ApiError.of(
                ErtugrulException.Code.VALIDATION_FAILED,
                "Request body could not be parsed",
                req.getRequestURI(), traceId));
    }

    @ExceptionHandler(MaxUploadSizeExceededException.class)
    public ResponseEntity<ApiError> handleTooLarge(MaxUploadSizeExceededException ex,
                                                   HttpServletRequest req) {
        String traceId = traceId();
        log.warn("[{}] upload too large at {}", traceId, req.getRequestURI());
        return ResponseEntity.status(HttpStatus.PAYLOAD_TOO_LARGE).body(ApiError.of(
                ErtugrulException.Code.SAVE_TOO_LARGE,
                "Uploaded payload exceeds the configured limit",
                req.getRequestURI(), traceId));
    }

    @ExceptionHandler(AccessDeniedException.class)
    public ResponseEntity<ApiError> handleAccessDenied(AccessDeniedException ex,
                                                       HttpServletRequest req) {
        String traceId = traceId();
        log.warn("[{}] access denied at {}", traceId, req.getRequestURI());
        return ResponseEntity.status(HttpStatus.FORBIDDEN).body(ApiError.of(
                ErtugrulException.Code.ACCOUNT_SUSPENDED,
                "Access denied",
                req.getRequestURI(), traceId));
    }

    @ExceptionHandler(NoHandlerFoundException.class)
    public ResponseEntity<ApiError> handleNotFound(NoHandlerFoundException ex,
                                                   HttpServletRequest req) {
        String traceId = traceId();
        return ResponseEntity.status(HttpStatus.NOT_FOUND).body(ApiError.of(
                ErtugrulException.Code.VALIDATION_FAILED,
                "No handler for " + req.getRequestURI(),
                req.getRequestURI(), traceId));
    }

    /**
     * Catch-all. Anything landing here is a bug: log with full stack, return a
     * body that leaks nothing.
     */
    @ExceptionHandler(Exception.class)
    public ResponseEntity<ApiError> handleUnexpected(Exception ex, HttpServletRequest req) {
        String traceId = traceId();
        log.error("[{}] unhandled exception at {}", traceId, req.getRequestURI(), ex);
        return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).body(ApiError.of(
                ErtugrulException.Code.INTERNAL,
                "Internal error",
                req.getRequestURI(), traceId));
    }

    // ── helpers ─────────────────────────────────────────────────────────────

    /**
     * Correlation id. Uses the current span when tracing is wired up, otherwise
     * a short random token — enough to grep the logs with.
     */
    private static String traceId() {
        String mdc = org.slf4j.MDC.get("traceId");
        if (mdc != null && !mdc.isBlank()) {
            return mdc;
        }
        return Long.toHexString(System.nanoTime());
    }

    private static String truncate(Object value) {
        if (value == null) {
            return null;
        }
        String s = value.toString();
        return s.length() <= MAX_REJECTED_LEN ? s : s.substring(0, MAX_REJECTED_LEN) + "...";
    }
}
