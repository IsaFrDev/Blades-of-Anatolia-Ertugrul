package com.fayzinc.ertugrul.journey;

import com.fayzinc.ertugrul.common.ErtugrulException;
import com.fayzinc.ertugrul.config.ErtugrulProperties;
import com.fayzinc.ertugrul.identity.Player;
import com.fayzinc.ertugrul.identity.PlayerRepository;
import com.fayzinc.ertugrul.journey.dto.ExportStatusResponse;
import com.lowagie.text.Document;
import com.lowagie.text.Element;
import com.lowagie.text.Font;
import com.lowagie.text.FontFactory;
import com.lowagie.text.PageSize;
import com.lowagie.text.Paragraph;
import com.lowagie.text.pdf.PdfWriter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import software.amazon.awssdk.core.sync.RequestBody;
import software.amazon.awssdk.services.s3.S3Client;
import software.amazon.awssdk.services.s3.model.GetObjectRequest;
import software.amazon.awssdk.services.s3.model.PutObjectRequest;
import software.amazon.awssdk.services.s3.presigner.S3Presigner;
import software.amazon.awssdk.services.s3.presigner.model.GetObjectPresignRequest;

import java.awt.Color;
import java.io.ByteArrayOutputStream;
import java.time.Instant;
import java.util.List;
import java.util.UUID;

/**
 * Generates PDF exports of the Safar Daftari.
 *
 * <p><b>Nima uchun bu marketing xususiyati.</b> O'yinchi 48 epizod davomida
 * o'z daftarini to'playdi, so'ng uni PDF qilib eksport qiladi va ijtimoiy
 * tarmoqda ulashadi. Bu — bepul, ishonchli va o'ziga xos reklama: har bir
 * daftar boshqacha, chunki har bir o'yinchining yo'li boshqacha.
 *
 * <p><b>Nima uchun asinxron.</b> To'liq daftar o'nlab sahifa. Uni so'rov
 * ichida chizish HTTP thread'ini sekundlab band qiladi. Shuning uchun ish
 * navbatga qo'yiladi, worker uni bajaradi, klient holatini so'rab turadi.
 *
 * <p><b>Tipografiya — bejiz emas.</b> EP024 dan keyingi sahifalar boshqa,
 * beqaror qo'lyozma bilan chiziladi. O'yinchi o'z daftarini qayta o'qiganda
 * qaysi sahifadan keyin qo'l o'zgarganini <i>ko'radi</i>. Jarohat tizimining
 * butun ma'nosi — o'yinchi tanasida qolgan tarix — shu bitta vizual detalda
 * yakunlanadi.
 */
@Service
public class JourneyExportService {

    private static final Logger log = LoggerFactory.getLogger(JourneyExportService.class);

    /** Ink colours per tone. Muted, period-appropriate, readable in print. */
    private static final Color INK_DEFAULT = new Color(0x2B, 0x24, 0x1B);
    private static final Color INK_WOUNDED = new Color(0x6B, 0x2A, 0x22);
    private static final Color INK_GRIEVING = new Color(0x3A, 0x3A, 0x4A);
    private static final Color ACCENT_GOLD = new Color(0xD4, 0xA8, 0x53);

    private final JourneyRepository.JourneyEntryRepository entryRepository;
    private final JourneyRepository.JourneyExportRepository exportRepository;
    private final PlayerRepository playerRepository;
    private final S3Client s3;
    private final S3Presigner presigner;
    private final ErtugrulProperties props;

    public JourneyExportService(JourneyRepository.JourneyEntryRepository entryRepository,
                                JourneyRepository.JourneyExportRepository exportRepository,
                                PlayerRepository playerRepository,
                                S3Client s3,
                                S3Presigner presigner,
                                ErtugrulProperties props) {
        this.entryRepository = entryRepository;
        this.exportRepository = exportRepository;
        this.playerRepository = playerRepository;
        this.s3 = s3;
        this.presigner = presigner;
        this.props = props;
    }

    /**
     * Eksport ishini navbatga qo'yadi.
     *
     * <p>Darhol {@code PENDING} qator qaytaradi; haqiqiy generatsiya
     * {@link #runExport} da, alohida thread'da bajariladi.
     *
     * @param playerId      who asked
     * @param playthroughId which diary to export
     * @param format        PDF or PNG
     * @return the queued job; poll it for completion
     * @throws ErtugrulException when the diary is empty
     */
    @Transactional
    public ExportStatusResponse requestExport(UUID playerId, UUID playthroughId,
                                              JourneyExport.Format format) {

        Player player = playerRepository.findById(playerId)
                .orElseThrow(() -> ErtugrulException.playerNotFound(playerId));

        List<JourneyEntry> entries = entryRepository
                .findByPlayerIdAndPlaythroughIdOrderBySequenceNoAsc(playerId, playthroughId);

        if (entries.isEmpty()) {
            throw new ErtugrulException(ErtugrulException.Code.JOURNEY_ENTRY_NOT_FOUND,
                    "This diary has no pages to export");
        }

        JourneyExport export = exportRepository.save(new JourneyExport(
                player, playthroughId, format,
                Instant.now().plus(props.journey().exportTtl())));

        log.info("Queued journey export={} player={} pages={}",
                export.getId(), playerId, entries.size());

        // No in-process hand-off here on purpose. JourneyExportWorker polls for
        // PENDING rows, which means a job survives a restart between the request
        // and the render — an in-process call would simply lose it.
        return ExportStatusResponse.from(export, null);
    }

    /**
     * Daftarni chizadi va object store'ga yuklaydi.
     *
     * <p>{@link JourneyExportWorker} chaqiradi. Hech qachon chaqiruvchiga xato
     * tashlamaydi: eksport <i>qo'shimcha</i> xususiyat, uning yiqilishi boshqa
     * hech narsani buzmasligi kerak. Xato {@code FAILED} holati sifatida
     * yoziladi va klient uni ko'radi.
     *
     * @param exportId the queued job
     */
    @Transactional
    public void processExport(UUID exportId) {
        JourneyExport export = exportRepository.findById(exportId).orElse(null);
        if (export == null) {
            log.warn("Export job {} vanished before it could run", exportId);
            return;
        }

        try {
            export.markRunning();

            List<JourneyEntry> entries = entryRepository
                    .findByPlayerIdAndPlaythroughIdOrderBySequenceNoAsc(
                            export.getPlayer().getId(), export.getPlaythroughId());

            byte[] document = renderPdf(entries);

            String objectKey = "exports/%s/%s.pdf".formatted(
                    export.getPlayer().getId(), export.getId());

            s3.putObject(PutObjectRequest.builder()
                            .bucket(props.s3().exportBucket())
                            .key(objectKey)
                            .contentType("application/pdf")
                            .contentLength((long) document.length)
                            .build(),
                    RequestBody.fromBytes(document));

            export.markReady(objectKey, document.length, entries.size());
            log.info("Journey export {} ready: {} pages, {} bytes",
                    exportId, entries.size(), document.length);

        } catch (Exception e) {
            log.error("Journey export {} failed", exportId, e);
            export.markFailed(e.getClass().getSimpleName() + ": " + e.getMessage());
        }
    }

    /**
     * Eksport holatini qaytaradi.
     *
     * @param playerId the owner
     * @param exportId the job
     * @return status, with a presigned download URL once ready
     */
    @Transactional(readOnly = true)
    public ExportStatusResponse status(UUID playerId, UUID exportId) {
        JourneyExport export = exportRepository.findById(exportId)
                .orElseThrow(() -> new ErtugrulException(ErtugrulException.Code.JOURNEY_ENTRY_NOT_FOUND,
                        "No such export"));

        // Ownership check: export ids are UUIDs, but "unguessable" is not the
        // same as "authorised".
        if (!export.getPlayer().getId().equals(playerId)) {
            throw new ErtugrulException(ErtugrulException.Code.JOURNEY_ENTRY_NOT_FOUND, "No such export");
        }

        String url = export.isReady() ? presignedExportUrl(export.getObjectKey()) : null;
        return ExportStatusResponse.from(export, url);
    }

    // ── PDF rendering ───────────────────────────────────────────────────────

    /**
     * Daftarni PDF ga chizadi.
     *
     * <p>Har sahifa: ikki taqvimli sarlavha, joy nomi, matn va kodeks
     * havolalari. Siyoh rangi {@link JourneyEntry.Tone} ga qarab tanlanadi,
     * EP024 dan keyingi sahifalar esa qiya (italic) yozuvda — beqaror chap
     * qo'l yozuvini ifodalaydi.
     *
     * @param entries the pages, already ordered
     * @return the rendered PDF bytes
     */
    private byte[] renderPdf(List<JourneyEntry> entries) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(64 * 1024);

        Document document = new Document(PageSize.A5, 42, 42, 54, 54);
        PdfWriter.getInstance(document, out);

        document.addTitle("Safar Daftari");
        document.addCreator("Dirilis: The Last March");
        document.open();

        writeCoverPage(document, entries);

        for (JourneyEntry entry : entries) {
            document.newPage();
            writeDiaryPage(document, entry);
        }

        document.close();
        return out.toByteArray();
    }

    private void writeCoverPage(Document document, List<JourneyEntry> entries) {
        Font titleFont = FontFactory.getFont(FontFactory.TIMES_BOLD, 26, ACCENT_GOLD);
        Font subtitleFont = FontFactory.getFont(FontFactory.TIMES_ITALIC, 13, INK_DEFAULT);

        Paragraph title = new Paragraph("SAFAR DAFTARI", titleFont);
        title.setAlignment(Element.ALIGN_CENTER);
        title.setSpacingBefore(140);
        document.add(title);

        JourneyEntry first = entries.get(0);
        JourneyEntry last = entries.get(entries.size() - 1);

        Paragraph span = new Paragraph(
                "%s  —  %s".formatted(first.getGregorianDateText(), last.getGregorianDateText()),
                subtitleFont);
        span.setAlignment(Element.ALIGN_CENTER);
        span.setSpacingBefore(18);
        document.add(span);

        Paragraph count = new Paragraph("%d sahifa".formatted(entries.size()), subtitleFont);
        count.setAlignment(Element.ALIGN_CENTER);
        count.setSpacingBefore(6);
        document.add(count);
    }

    private void writeDiaryPage(Document document, JourneyEntry entry) {
        Color ink = inkFor(entry.getTone());

        // The wound made visible: post-EP024 pages are written left-handed, and
        // the italic face stands in for that unsteadiness. This one detail is
        // what makes the exported diary feel like an artefact of a life rather
        // than a log file.
        String bodyFace = entry.isWrittenLeftHanded()
                ? FontFactory.TIMES_ITALIC
                : FontFactory.TIMES_ROMAN;

        Font headingFont = FontFactory.getFont(FontFactory.TIMES_BOLD, 12, ACCENT_GOLD);
        Font placeFont = FontFactory.getFont(FontFactory.TIMES_ITALIC, 10, ink);
        Font bodyFont = FontFactory.getFont(bodyFace, 11.5f, ink);
        Font linkFont = FontFactory.getFont(FontFactory.TIMES_ITALIC, 9, ACCENT_GOLD);

        Paragraph heading = new Paragraph(entry.dualDateHeading(), headingFont);
        heading.setSpacingAfter(4);
        document.add(heading);

        if (entry.getPlaceName() != null && !entry.getPlaceName().isBlank()) {
            Paragraph place = new Paragraph(entry.getPlaceName().toUpperCase(), placeFont);
            place.setSpacingAfter(14);
            document.add(place);
        }

        Paragraph body = new Paragraph(entry.getBody(), bodyFont);
        body.setAlignment(Element.ALIGN_JUSTIFIED);
        body.setLeading(17f);
        body.setSpacingAfter(16);
        document.add(body);

        if (!entry.getLinkedCodexIds().isEmpty()) {
            // Rendered as the codex ids themselves: resolving them to titles would
            // mean fetching the CDN catalogue in the player's language, and an
            // export must not fail because a content CDN is slow.
            Paragraph links = new Paragraph(
                    entry.getLinkedCodexIds().stream()
                            .map(id -> "[" + id.replace("CDX_", "").replace('_', ' ') + "]")
                            .reduce((a, b) -> a + "  " + b)
                            .orElse(""),
                    linkFont);
            document.add(links);
        }
    }

    private static Color inkFor(JourneyEntry.Tone tone) {
        return switch (tone) {
            case WOUNDED -> INK_WOUNDED;
            case GRIEVING -> INK_GRIEVING;
            case NEUTRAL, HOPEFUL, RESOLUTE -> INK_DEFAULT;
        };
    }

    private String presignedExportUrl(String objectKey) {
        GetObjectPresignRequest request = GetObjectPresignRequest.builder()
                .signatureDuration(props.s3().presignTtl())
                .getObjectRequest(GetObjectRequest.builder()
                        .bucket(props.s3().exportBucket())
                        .key(objectKey)
                        .build())
                .build();

        return presigner.presignGetObject(request).url().toString();
    }
}
