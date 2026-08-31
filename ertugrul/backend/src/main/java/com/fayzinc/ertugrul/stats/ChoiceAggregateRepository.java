package com.fayzinc.ertugrul.stats;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface ChoiceAggregateRepository
        extends JpaRepository<ChoiceAggregate, ChoiceAggregate.ChoiceKey> {

    /** Every option of one choice — the split shown after an episode. */
    List<ChoiceAggregate> findByKeyChoiceId(String choiceId);

    List<ChoiceAggregate> findByEpisodeId(String episodeId);

    /** All seven uncertainty scenes at once, for the end-of-game summary screen. */
    List<ChoiceAggregate> findByUncertaintySceneTrue();

    /**
     * Total votes cast for a choice.
     *
     * <p>Foizni hisoblash uchun kerak. Alohida so'rov sifatida, chunki
     * yig'indini Java tomonida hisoblash — barcha qatorlarni tortib olishni
     * talab qiladi, bu esa faqat foiz kerak bo'lgan holatda isrof.
     */
    @Query("""
            select coalesce(sum(a.pickCount), 0)
            from ChoiceAggregate a
            where a.key.choiceId = :choiceId
            """)
    long totalVotesFor(@Param("choiceId") String choiceId);
}
