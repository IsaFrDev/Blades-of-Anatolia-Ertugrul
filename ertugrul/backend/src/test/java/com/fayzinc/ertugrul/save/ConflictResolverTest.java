package com.fayzinc.ertugrul.save;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;

import java.time.Instant;
import java.util.Map;

import static org.assertj.core.api.Assertions.assertThat;

/**
 * Tests for the cloud-save conflict logic.
 *
 * <p>Bu — butun servisdagi eng nozik mantiq, va uning xatosi eng qimmat:
 * noto'g'ri hal qilingan konflikt o'yinchining soatlab progressini
 * yo'qotadi. Shuning uchun har bir holat alohida tekshiriladi.
 */
class ConflictResolverTest {

    private static final String PC = "device-pc-01";
    private static final String PS5 = "device-ps5-02";

    private final ConflictResolver resolver = new ConflictResolver();

    @Nested
    @DisplayName("VectorClock relations")
    class ClockRelations {

        @Test
        @DisplayName("identical clocks are IDENTICAL")
        void identical() {
            VectorClock a = VectorClock.of(Map.of(PC, 5L, PS5, 2L));
            VectorClock b = VectorClock.of(Map.of(PC, 5L, PS5, 2L));

            assertThat(a.relationTo(b)).isEqualTo(VectorClock.Relation.IDENTICAL);
        }

        @Test
        @DisplayName("a clock that has seen strictly more DESCENDS")
        void descends() {
            VectorClock ahead = VectorClock.of(Map.of(PC, 6L, PS5, 2L));
            VectorClock behind = VectorClock.of(Map.of(PC, 5L, PS5, 2L));

            assertThat(ahead.relationTo(behind)).isEqualTo(VectorClock.Relation.DESCENDS);
            assertThat(behind.relationTo(ahead)).isEqualTo(VectorClock.Relation.PRECEDES);
        }

        @Test
        @DisplayName("a node missing from a clock counts as zero, not as a conflict")
        void missingNodeIsZero() {
            VectorClock withBoth = VectorClock.of(Map.of(PC, 3L, PS5, 1L));
            VectorClock pcOnly = VectorClock.of(Map.of(PC, 3L));

            assertThat(withBoth.relationTo(pcOnly)).isEqualTo(VectorClock.Relation.DESCENDS);
        }

        @Test
        @DisplayName("two devices that each advanced independently are CONCURRENT")
        void concurrent() {
            // The real scenario: PC played on to EP023 while an offline PS5
            // carried on from EP021. Neither has seen the other's writes.
            VectorClock pc = VectorClock.of(Map.of(PC, 8L, PS5, 2L));
            VectorClock ps5 = VectorClock.of(Map.of(PC, 5L, PS5, 4L));

            assertThat(pc.relationTo(ps5)).isEqualTo(VectorClock.Relation.CONCURRENT);
            assertThat(ps5.relationTo(pc)).isEqualTo(VectorClock.Relation.CONCURRENT);
        }

        @Test
        @DisplayName("merge takes the pointwise maximum")
        void mergeTakesMaximum() {
            VectorClock pc = VectorClock.of(Map.of(PC, 8L, PS5, 2L));
            VectorClock ps5 = VectorClock.of(Map.of(PC, 5L, PS5, 4L));

            VectorClock merged = pc.merge(ps5);

            assertThat(merged.counterFor(PC)).isEqualTo(8L);
            assertThat(merged.counterFor(PS5)).isEqualTo(4L);
        }

        @Test
        @DisplayName("increment advances only the writing device")
        void incrementIsPerDevice() {
            VectorClock before = VectorClock.of(Map.of(PC, 3L, PS5, 1L));
            VectorClock after = before.increment(PC);

            assertThat(after.counterFor(PC)).isEqualTo(4L);
            assertThat(after.counterFor(PS5)).isEqualTo(1L);
            // Immutability: the original is untouched.
            assertThat(before.counterFor(PC)).isEqualTo(3L);
        }

        @Test
        @DisplayName("compaction drops the least active nodes")
        void compactionKeepsBusiestNodes() {
            Map<String, Long> many = new java.util.HashMap<>();
            for (int i = 0; i < VectorClock.MAX_NODES + 5; i++) {
                many.put("device-" + i, (long) i);
            }

            VectorClock compacted = VectorClock.of(many).compact();

            assertThat(compacted.counters()).hasSize(VectorClock.MAX_NODES);
            // The highest counter must survive; the lowest must not.
            assertThat(compacted.counters()).containsKey("device-" + (VectorClock.MAX_NODES + 4));
            assertThat(compacted.counters()).doesNotContainKey("device-0");
        }
    }

    @Nested
    @DisplayName("Resolution outcomes")
    class Resolution {

        private final Instant now = Instant.parse("2026-08-27T12:00:00Z");

        @Test
        @DisplayName("an empty slot accepts the first save")
        void emptySlotAcceptsInitial() {
            ConflictResolver.Decision decision = resolver.resolve(
                    VectorClock.of(Map.of(PC, 1L)),
                    VectorClock.empty(),
                    now, Instant.EPOCH,
                    ConflictResolver.ProgressScore.of(1, 600),
                    ConflictResolver.ProgressScore.of(0, 0));

            assertThat(decision.outcome()).isEqualTo(ConflictResolver.Outcome.ACCEPT_INITIAL);
            assertThat(decision.outcome().becomesHead()).isTrue();
        }

        @Test
        @DisplayName("a strictly newer clock fast-forwards")
        void fastForward() {
            ConflictResolver.Decision decision = resolver.resolve(
                    VectorClock.of(Map.of(PC, 6L)),
                    VectorClock.of(Map.of(PC, 5L)),
                    now, now.minusSeconds(300),
                    ConflictResolver.ProgressScore.of(12, 40_000),
                    ConflictResolver.ProgressScore.of(11, 38_000));

            assertThat(decision.outcome()).isEqualTo(ConflictResolver.Outcome.ACCEPT_FAST_FORWARD);
            assertThat(decision.outcome().isConflict()).isFalse();
        }

        @Test
        @DisplayName("a client that is behind is told it is stale")
        void staleUploadRejected() {
            ConflictResolver.Decision decision = resolver.resolve(
                    VectorClock.of(Map.of(PC, 4L)),
                    VectorClock.of(Map.of(PC, 7L)),
                    now, now.minusSeconds(60),
                    ConflictResolver.ProgressScore.of(10, 30_000),
                    ConflictResolver.ProgressScore.of(12, 40_000));

            assertThat(decision.outcome()).isEqualTo(ConflictResolver.Outcome.REJECT_STALE);
            assertThat(decision.outcome().becomesHead()).isFalse();
        }

        @Test
        @DisplayName("divergent histories are settled on progress, not on the clock")
        void concurrentResolvedByProgress() {
            // The incoming save is OLDER by wall clock but FURTHER along. A naive
            // last-write-wins would discard four episodes of progress here.
            ConflictResolver.Decision decision = resolver.resolve(
                    VectorClock.of(Map.of(PC, 8L, PS5, 2L)),
                    VectorClock.of(Map.of(PC, 5L, PS5, 4L)),
                    now.minusSeconds(3600),   // incoming saved an hour earlier
                    now,                      // head saved just now
                    ConflictResolver.ProgressScore.of(23, 90_000),
                    ConflictResolver.ProgressScore.of(19, 70_000));

            assertThat(decision.outcome())
                    .isEqualTo(ConflictResolver.Outcome.CONFLICT_INCOMING_WINS);
            assertThat(decision.reason()).contains("progress");
        }

        @Test
        @DisplayName("equal progress falls back to last-write-wins")
        void concurrentResolvedByTimestamp() {
            ConflictResolver.Decision decision = resolver.resolve(
                    VectorClock.of(Map.of(PC, 8L, PS5, 2L)),
                    VectorClock.of(Map.of(PC, 5L, PS5, 4L)),
                    now,
                    now.minusSeconds(600),
                    ConflictResolver.ProgressScore.of(20, 80_000),
                    ConflictResolver.ProgressScore.of(20, 80_000));

            assertThat(decision.outcome())
                    .isEqualTo(ConflictResolver.Outcome.CONFLICT_INCOMING_WINS);
            assertThat(decision.reason()).contains("last-write-wins");
        }

        @Test
        @DisplayName("a fully tied conflict resolves deterministically")
        void tiedConflictIsDeterministic() {
            VectorClock incoming = VectorClock.of(Map.of(PC, 8L, PS5, 2L));
            VectorClock head = VectorClock.of(Map.of(PC, 5L, PS5, 4L));
            ConflictResolver.ProgressScore same = ConflictResolver.ProgressScore.of(20, 80_000);

            // Identical inputs must always produce identical output: a client
            // retry must not flip the winner.
            ConflictResolver.Decision first = resolver.resolve(
                    incoming, head, now, now.plusSeconds(1), same, same);
            ConflictResolver.Decision second = resolver.resolve(
                    incoming, head, now, now.plusSeconds(1), same, same);

            assertThat(first.outcome()).isEqualTo(second.outcome());
            assertThat(first.outcome().isConflict()).isTrue();
        }

        @Test
        @DisplayName("the merged clock always carries both histories forward")
        void mergedClockRetainsBothSides() {
            ConflictResolver.Decision decision = resolver.resolve(
                    VectorClock.of(Map.of(PC, 8L, PS5, 2L)),
                    VectorClock.of(Map.of(PC, 5L, PS5, 4L)),
                    now, now,
                    ConflictResolver.ProgressScore.of(23, 90_000),
                    ConflictResolver.ProgressScore.of(19, 70_000));

            // Without the merge the loser's history would look "unseen" and the
            // very next upload would conflict all over again.
            assertThat(decision.mergedClock().counterFor(PC)).isEqualTo(8L);
            assertThat(decision.mergedClock().counterFor(PS5)).isEqualTo(4L);
        }
    }
}
