-- =============================================================================
-- V3 — Telemetry rollups, choice aggregates, and live-ops.
--
-- IMPORTANT: raw telemetry events are NOT stored in Postgres. They go to Kafka
-- (`ert.telemetry.raw`) and from there to the warehouse. Postgres holds only
-- the small rollups the live-ops dashboard and the game client read back.
-- Putting 48 episodes x millions of sessions of raw events in Postgres would
-- make it the bottleneck for a service whose actual job is saves.
-- =============================================================================

-- -----------------------------------------------------------------------------
-- Episode funnel. One row per (day, episode, difficulty). Written by the
-- telemetry consumer; read by the live-ops dashboard.
--
-- This is the table that answers 00_AUDIT.md D8: "where are players dropping".
-- -----------------------------------------------------------------------------
CREATE TABLE episode_funnel_daily (
    bucket_date         DATE         NOT NULL,
    episode_id          CHAR(5)      NOT NULL
        CONSTRAINT ck_funnel_episode CHECK (episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),
    difficulty_tier     VARCHAR(16)  NOT NULL DEFAULT 'ALP'
        CONSTRAINT ck_funnel_difficulty CHECK (difficulty_tier IN ('LEGEND', 'ALP', 'FRONTIER', 'CHRONICLE')),

    starts              BIGINT       NOT NULL DEFAULT 0,
    completes           BIGINT       NOT NULL DEFAULT 0,
    abandons            BIGINT       NOT NULL DEFAULT 0,
    deaths              BIGINT       NOT NULL DEFAULT 0,

    -- Sum + count instead of a stored average: rollups merge additively.
    duration_sum_sec    BIGINT       NOT NULL DEFAULT 0,
    duration_count      BIGINT       NOT NULL DEFAULT 0,

    -- Playable intros are a headline feature (06_INTERACTIVE_INTRO.md).
    -- If skips spike, the intro is not earning its 4 minutes.
    intro_skips         BIGINT       NOT NULL DEFAULT 0,

    updated_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT pk_episode_funnel PRIMARY KEY (bucket_date, episode_id, difficulty_tier)
);

COMMENT ON TABLE episode_funnel_daily IS 'Per-day episode funnel rollup. completes/starts is the retention signal.';

CREATE INDEX ix_funnel_episode ON episode_funnel_daily (episode_id, bucket_date DESC);


-- -----------------------------------------------------------------------------
-- Wound ("mix") balance telemetry — 05_MIH_SYSTEM.md §8.
--
-- The wound system is the game's signature mechanic AND its biggest balance
-- risk: it is designed to never fully heal, so a too-steep decay curve in one
-- episode turns into a wall the player cannot pass. This rollup is what lets
-- live-ops detect that WITHOUT a client patch, and push an
-- EpisodeBalanceOverride the same day.
--
-- Design threshold from the doc: avg hand_integrity < 15 AND avg deaths > 6
-- in an episode  =>  the system is too punishing there.
-- -----------------------------------------------------------------------------
CREATE TABLE wound_balance_daily (
    bucket_date             DATE         NOT NULL,
    episode_id              CHAR(5)      NOT NULL
        CONSTRAINT ck_wound_episode CHECK (episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),
    phase                   VARCHAR(8)   NOT NULL
        CONSTRAINT ck_wound_phase CHECK (phase IN ('INTACT', 'FRESH', 'CHRONIC', 'ADAPTED')),

    sample_count            BIGINT       NOT NULL DEFAULT 0,

    hand_integrity_sum      DOUBLE PRECISION NOT NULL DEFAULT 0,
    max_integrity_sum       DOUBLE PRECISION NOT NULL DEFAULT 0,
    sabr_sum                DOUBLE PRECISION NOT NULL DEFAULT 0,
    deaths_sum              BIGINT       NOT NULL DEFAULT 0,
    flashbacks_sum          BIGINT       NOT NULL DEFAULT 0,
    opium_uses_sum          BIGINT       NOT NULL DEFAULT 0,

    -- Count of samples below the "desperate" line, so we can see the tail and
    -- not just the mean — a bimodal episode hides behind a healthy average.
    below_15_count          BIGINT       NOT NULL DEFAULT 0,

    -- Raised by EpisodeFunnelService when the doc's threshold trips. Live-ops
    -- dashboard highlights these rows in red.
    flagged_too_punishing   BOOLEAN      NOT NULL DEFAULT FALSE,

    updated_at              TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT pk_wound_balance PRIMARY KEY (bucket_date, episode_id, phase)
);

COMMENT ON TABLE wound_balance_daily IS 'HandIntegrity/Sabr rollup. Drives automatic "too punishing" detection per episode.';
COMMENT ON COLUMN wound_balance_daily.below_15_count IS 'Tail counter — a bimodal episode hides behind a healthy mean.';

CREATE INDEX ix_wound_flagged ON wound_balance_daily (episode_id, bucket_date DESC)
    WHERE flagged_too_punishing;


-- -----------------------------------------------------------------------------
-- Choice aggregates — 02_HISTORY_LAYER.md §7.
--
-- NOT a competitive leaderboard. The player finishes an episode and sees
-- "62% of players spared Titus" — a mirror, not a scoreboard. Covers both the
-- 7 Uncertainty Scenes (SS_1..SS_7) and ordinary narrative choices.
-- -----------------------------------------------------------------------------
CREATE TABLE choice_aggregate (
    choice_id       VARCHAR(64)  NOT NULL,
    option_id       VARCHAR(64)  NOT NULL,

    episode_id      CHAR(5)      NOT NULL
        CONSTRAINT ck_choice_episode CHECK (episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),

    -- TRUE for SS_1..SS_7. These get a dedicated UI treatment ("historians
    -- disagree, and so do players") so they are flagged rather than inferred.
    uncertainty_scene BOOLEAN    NOT NULL DEFAULT FALSE,

    -- Counted once per player per choice: the dedupe happens upstream in the
    -- consumer, because a player replaying in NG+ must not skew the split.
    pick_count      BIGINT       NOT NULL DEFAULT 0
        CONSTRAINT ck_choice_count CHECK (pick_count >= 0),

    updated_at      TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT pk_choice_aggregate PRIMARY KEY (choice_id, option_id)
);

COMMENT ON TABLE choice_aggregate IS 'Global "how did your choices compare" splits. Excludes players over the anti-cheat threshold.';
COMMENT ON COLUMN choice_aggregate.uncertainty_scene IS 'SS_1..SS_7 — competing historical interpretations, shown with scholar attribution.';

CREATE INDEX ix_choice_episode ON choice_aggregate (episode_id);
CREATE INDEX ix_choice_scene   ON choice_aggregate (choice_id) WHERE uncertainty_scene;

-- Dedupe ledger: which players have already been counted for which choice.
-- Small and narrow on purpose — it is only an existence check.
CREATE TABLE choice_vote_ledger (
    player_id   UUID         NOT NULL REFERENCES player (id) ON DELETE CASCADE,
    choice_id   VARCHAR(64)  NOT NULL,
    option_id   VARCHAR(64)  NOT NULL,
    recorded_at TIMESTAMPTZ  NOT NULL DEFAULT now(),
    CONSTRAINT pk_choice_vote PRIMARY KEY (player_id, choice_id)
);

COMMENT ON TABLE choice_vote_ledger IS 'First choice per player wins; NG+ replays do not re-skew the global split.';


-- =============================================================================
-- Live-ops
-- =============================================================================

-- -----------------------------------------------------------------------------
-- Remote config: arbitrary tunables delivered without a client patch.
-- -----------------------------------------------------------------------------
CREATE TABLE remote_config (
    id              UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    config_key      VARCHAR(96)  NOT NULL,

    -- Arbitrary JSON. The client validates against its own schema and falls
    -- back to the shipped default if the shape is unknown — a bad remote config
    -- must never brick a single-player game.
    config_value    JSONB        NOT NULL,

    -- Rollout cohort: 0 = off, 100 = everyone. Bucketing is a stable hash of
    -- (playerId + cohort_salt), so a player never flips cohort between sessions.
    rollout_percent SMALLINT     NOT NULL DEFAULT 100
        CONSTRAINT ck_config_rollout CHECK (rollout_percent BETWEEN 0 AND 100),

    -- Optional targeting.
    min_app_version VARCHAR(32),
    platform        VARCHAR(16),

    active_from     TIMESTAMPTZ  NOT NULL DEFAULT now(),
    active_until    TIMESTAMPTZ,

    -- Bumped on every edit; the client sends it back as an ETag so an unchanged
    -- config costs a 304 instead of a payload.
    revision        BIGINT       NOT NULL DEFAULT 1,

    updated_by      VARCHAR(64),
    updated_at      TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_remote_config_key UNIQUE (config_key)
);

COMMENT ON COLUMN remote_config.rollout_percent IS 'Stable-hash cohort. Changing ertugrul.liveops.cohort-salt reshuffles every assignment.';

CREATE INDEX ix_config_active ON remote_config (active_from, active_until);


-- -----------------------------------------------------------------------------
-- Per-episode balance overrides — the sharp end of live-ops.
--
-- These map 1:1 onto UErtGameUserSettings fields on the client
-- (07_SETTINGS_HOTKEYS.md §5.3), so an override is applied by the same code
-- path as a player-chosen accessibility setting. That is deliberate: it is
-- already tested, and it means an override can never reach a value the game
-- cannot render.
-- -----------------------------------------------------------------------------
CREATE TABLE episode_balance_override (
    id                    UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    episode_id            CHAR(5)      NOT NULL
        CONSTRAINT ck_override_episode CHECK (episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),

    -- Optional: an override may target one difficulty tier only. NULL = all.
    difficulty_tier       VARCHAR(16)
        CONSTRAINT ck_override_difficulty CHECK (difficulty_tier IS NULL OR difficulty_tier IN
            ('LEGEND', 'ALP', 'FRONTIER', 'CHRONICLE')),

    -- Parry window is the heart of combat (04_CORE_SYSTEMS.md §1.3): base 180ms
    -- healthy, 110-165ms after the nail. Bounded so live-ops cannot make parry
    -- impossible or trivial.
    parry_window_ms_delta INT          NOT NULL DEFAULT 0
        CONSTRAINT ck_override_parry CHECK (parry_window_ms_delta BETWEEN -60 AND 120),

    enemy_damage_scale    REAL         NOT NULL DEFAULT 1.0
        CONSTRAINT ck_override_damage CHECK (enemy_damage_scale BETWEEN 0.25 AND 2.5),

    -- The wound decay multiplier. This is the lever pulled when
    -- wound_balance_daily flags an episode as too punishing.
    wound_decay_scale     REAL         NOT NULL DEFAULT 1.0
        CONSTRAINT ck_override_wound CHECK (wound_decay_scale BETWEEN 0.2 AND 2.0),

    enemy_count_scale     REAL         NOT NULL DEFAULT 1.0
        CONSTRAINT ck_override_count CHECK (enemy_count_scale BETWEEN 0.5 AND 2.0),

    -- A/B rollout. Two overrides on the same episode with disjoint cohorts is
    -- how a balance change is validated before going to 100%.
    rollout_percent       SMALLINT     NOT NULL DEFAULT 100
        CONSTRAINT ck_override_rollout CHECK (rollout_percent BETWEEN 0 AND 100),
    -- Distinguishes A from B when two experiments share an episode.
    variant_key           VARCHAR(32)  NOT NULL DEFAULT 'default',

    enabled               BOOLEAN      NOT NULL DEFAULT TRUE,
    notes                 TEXT,

    active_from           TIMESTAMPTZ  NOT NULL DEFAULT now(),
    active_until          TIMESTAMPTZ,
    revision              BIGINT       NOT NULL DEFAULT 1,
    updated_by            VARCHAR(64),
    updated_at            TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_override_episode_variant UNIQUE (episode_id, variant_key)
);

COMMENT ON TABLE episode_balance_override IS 'Remote per-episode difficulty tuning. Bounds are enforced in SQL so a bad dashboard entry cannot brick an episode.';

CREATE INDEX ix_override_lookup ON episode_balance_override (episode_id)
    WHERE enabled;


-- -----------------------------------------------------------------------------
-- Seasonal events: time-boxed content windows (anniversary of Kose Dag, a
-- Discovery Tour weekend, a new codex pack drop).
-- -----------------------------------------------------------------------------
CREATE TABLE seasonal_event (
    id              UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    event_key       VARCHAR(64)  NOT NULL,
    title_loc_key   VARCHAR(96)  NOT NULL,
    body_loc_key    VARCHAR(96),

    event_type      VARCHAR(24)  NOT NULL
        CONSTRAINT ck_event_type CHECK (event_type IN
            ('CODEX_DROP', 'DISCOVERY_TOUR', 'ANNIVERSARY', 'BALANCE_WEEKEND', 'ANNOUNCEMENT')),

    -- For CODEX_DROP: the CDN manifest listing the new entries. Live-ops adds
    -- codex entries without a client patch (02_HISTORY_LAYER.md §9).
    cdn_manifest_url VARCHAR(256),

    payload         JSONB        NOT NULL DEFAULT '{}'::jsonb,

    starts_at       TIMESTAMPTZ  NOT NULL,
    ends_at         TIMESTAMPTZ  NOT NULL,
    enabled         BOOLEAN      NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_event_key UNIQUE (event_key),
    CONSTRAINT ck_event_window CHECK (ends_at > starts_at)
);

CREATE INDEX ix_event_window ON seasonal_event (starts_at, ends_at) WHERE enabled;


-- -----------------------------------------------------------------------------
-- Rejected telemetry. Sanity-check failures are kept (bounded, pruned weekly)
-- because a sudden spike of one rejection reason is usually a client bug, not
-- a cheater.
-- -----------------------------------------------------------------------------
CREATE TABLE telemetry_rejection (
    id              BIGINT       GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    player_id       UUID         REFERENCES player (id) ON DELETE SET NULL,
    event_type      VARCHAR(24)  NOT NULL,
    episode_id      CHAR(5),
    reason          VARCHAR(64)  NOT NULL,
    detail          TEXT,
    app_version     VARCHAR(32),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE INDEX ix_rejection_reason ON telemetry_rejection (reason, created_at DESC);
CREATE INDEX ix_rejection_player ON telemetry_rejection (player_id, created_at DESC);
