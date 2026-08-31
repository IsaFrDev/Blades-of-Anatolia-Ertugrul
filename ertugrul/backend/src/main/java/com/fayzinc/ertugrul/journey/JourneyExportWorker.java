package com.fayzinc.ertugrul.journey;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.domain.Limit;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import java.time.Duration;
import java.time.Instant;
import java.util.List;

/**
 * Picks up queued diary exports and renders them.
 *
 * <p><b>Nega poller, in-process hand-off emas.</b> Agar eksport so'rovdan
 * keyin darhol {@code @Async} bilan chaqirilsa, server o'sha lahzada qayta
 * ishga tushsa ish <i>butunlay yo'qoladi</i> — qator {@code PENDING} da
 * abadiy qolib ketadi va o'yinchi kutib o'tiraveradi. Baza — navbatning
 * o'zi; poller esa uni bo'shatadi. Bu sekinroq, lekin ishonchli.
 *
 * <p><b>Osilib qolgan ishlar.</b> {@code RUNNING} da qolgan ish ham qayta
 * olinadi: agar worker generatsiya paytida yiqilgan bo'lsa, qator o'sha
 * holatda qotib qoladi. {@link #STALE_AFTER} — ishning eng uzun ishonarli
 * davomiyligi; undan oshgani yiqilgan deb hisoblanadi.
 */
@Component
public class JourneyExportWorker {

    private static final Logger log = LoggerFactory.getLogger(JourneyExportWorker.class);

    /** A job still RUNNING after this long is assumed dead and retried. */
    private static final Duration STALE_AFTER = Duration.ofMinutes(5);

    /** Jobs claimed per tick. Bounded so one instance cannot monopolise the queue. */
    private static final int BATCH_SIZE = 10;

    private final JourneyRepository.JourneyExportRepository exportRepository;
    private final JourneyExportService exportService;

    public JourneyExportWorker(JourneyRepository.JourneyExportRepository exportRepository,
                               JourneyExportService exportService) {
        this.exportRepository = exportRepository;
        this.exportService = exportService;
    }

    /**
     * Navbatdagi eksportlarni bajaradi.
     *
     * <p>Har 15 soniyada ishlaydi. O'yinchi PDF ni bir necha soniya kutishga
     * tayyor — u shu zahoti kerak bo'ladigan narsa emas, ulashish uchun
     * tayyorlanayotgan hujjat.
     */
    @Scheduled(fixedDelayString = "PT15S")
    public void drainQueue() {
        // PENDING is claimed immediately; RUNNING only once it has been stuck
        // long enough to count as a crashed worker rather than a busy one.
        List<JourneyExport> claimable = exportRepository.findClaimable(
                JourneyExport.Status.PENDING,
                JourneyExport.Status.RUNNING,
                Instant.now().minus(STALE_AFTER),
                Limit.of(BATCH_SIZE));

        if (claimable.isEmpty()) {
            return;
        }

        log.debug("Journey export worker claimed {} job(s)", claimable.size());

        for (JourneyExport export : claimable) {
            try {
                // Separate bean, so @Transactional on processExport genuinely
                // applies and each job commits or rolls back on its own.
                exportService.processExport(export.getId());
            } catch (Exception e) {
                // One bad job must not stop the queue.
                log.error("Journey export {} threw out of processExport", export.getId(), e);
            }
        }
    }
}
