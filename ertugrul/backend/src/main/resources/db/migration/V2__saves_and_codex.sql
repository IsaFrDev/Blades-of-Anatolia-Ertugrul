-- =============================================================================
-- V2 — Cloud save (versioned blobs + vector clocks) and codex progress.
--
-- Storage split:
--   * The blob itself (world_state, hand state, faction reputations, quest
--     graph) lives in S3/MinIO as an immutable versioned object.
--   * Postgres holds metadata, the vector clock, and a small denormalised
--     summary so the "Continue" screen renders without touching S3.
--
-- Why vector clocks: the player may finish EP023 on PC while an offline PS5
-- still holds an EP021 save. Wall-clock alone cannot tell "newer" from
-- "divergent", and silently discarding 40 minutes of a single-player campaign
-- is unacceptable. So: vector clock decides; when it says CONCURRENT we fall
-- back to last-write-wins BUT keep the loser as a restorable conflict copy.
-- =============================================================================

CREATE TABLE save_slot (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id           UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,

    -- 0 = autosave (engine-driven), 1..8 = manual slots.
    slot_index          SMALLINT     NOT NULL
        CONSTRAINT ck_slot_index CHECK (slot_index BETWEEN 0 AND 8),

    -- Monotonic per slot. Points at the winning save_version.
    head_version        BIGINT       NOT NULL DEFAULT 0,

    -- Merged vector clock of the head: {"device-abc": 12, "device-xyz": 4}.
    -- Kept denormalised here so the pre-flight conflict check is a single read.
    vector_clock        JSONB        NOT NULL DEFAULT '{}'::jsonb,

    -- ── Denormalised summary for the load/continue screen ────────────────────
    episode_id          CHAR(5)
        CONSTRAINT ck_slot_episode CHECK (episode_id IS NULL OR episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),
    season_id           CHAR(2)
        CONSTRAINT ck_slot_season CHECK (season_id IS NULL OR season_id ~ '^S[1-4]$'),
    playtime_seconds    BIGINT       NOT NULL DEFAULT 0,

    -- The wound. Surfaced on the load screen as the hand icon (05_MIH_SYSTEM.md
    -- §7) — never as a number, but the client needs the value to pick the icon.
    hand_integrity      REAL         NOT NULL DEFAULT 100
        CONSTRAINT ck_slot_hand CHECK (hand_integrity BETWEEN 0 AND 100),
    max_integrity       REAL         NOT NULL DEFAULT 100
        CONSTRAINT ck_slot_maxint CHECK (max_integrity BETWEEN 0 AND 100),
    difficulty_tier     VARCHAR(16)
        CONSTRAINT ck_slot_difficulty CHECK (difficulty_tier IS NULL OR difficulty_tier IN
            ('LEGEND', 'ALP', 'FRONTIER', 'CHRONICLE')),

    -- Save-format version. Bumped when the blob layout changes; the client
    -- refuses to load a blob newer than it understands (00_AUDIT.md B1.1).
    schema_version      INT          NOT NULL DEFAULT 1,

    -- True while an unresolved conflict copy exists for this slot.
    has_conflict        BOOLEAN      NOT NULL DEFAULT FALSE,

    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_slot_player_index UNIQUE (player_id, slot_index)
);

COMMENT ON TABLE  save_slot IS 'One row per (player, slot). 8 manual slots + slot 0 autosave.';
COMMENT ON COLUMN save_slot.vector_clock IS 'device_id -> counter. Merged clock of head_version.';
COMMENT ON COLUMN save_slot.max_integrity IS 'Ceiling for hand_integrity. 100 until EP024, then 55; only the EP043 prosthesis raises it.';

CREATE INDEX ix_slot_player ON save_slot (player_id, slot_index);
CREATE INDEX ix_slot_conflict ON save_slot (player_id) WHERE has_conflict;


-- -----------------------------------------------------------------------------
-- Immutable version history. Never UPDATEd except to flip `superseded_at`.
-- Retention: newest N per slot (ertugrul.save.retained-versions-per-slot);
-- older rows and their S3 objects are pruned nightly.
-- -----------------------------------------------------------------------------
CREATE TABLE save_version (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    slot_id             UUID         NOT NULL
        REFERENCES save_slot (id) ON DELETE CASCADE,

    version             BIGINT       NOT NULL,

    -- S3 key: saves/{playerId}/{slotIndex}/{version}.sav
    object_key          VARCHAR(256) NOT NULL,
    size_bytes          INT          NOT NULL
        CONSTRAINT ck_version_size CHECK (size_bytes > 0 AND size_bytes <= 8388608),

    -- Content hash of the blob. Used for dedupe (a re-upload of an identical
    -- blob is a no-op) and for the server HMAC input.
    sha256              CHAR(64)     NOT NULL,

    -- Server-side HMAC over (playerId | slotIndex | version | sha256).
    -- Verified on download: catches object-store tampering and blob swapping.
    server_hmac         CHAR(64)     NOT NULL,

    -- Client-supplied HMAC. Soft signal only — the key ships inside the game
    -- binary and is therefore extractable. A mismatch bumps integrity_score.
    client_hmac_valid   BOOLEAN,

    vector_clock        JSONB        NOT NULL,

    -- The device that wrote it, and when the CLIENT thinks it wrote it.
    -- client_saved_at is untrusted: it is only the LWW tiebreak input, and it
    -- is rejected outright if it is further ahead than the allowed clock skew.
    origin_device_id    VARCHAR(64)  NOT NULL,
    client_saved_at     TIMESTAMPTZ  NOT NULL,

    -- Conflict bookkeeping. When LWW picks a winner, the loser is retained with
    -- conflict_lost_to pointing at the winner so support (or the player, via
    -- the "restore other version" UI) can recover it.
    conflict_lost_to    UUID         REFERENCES save_version (id) ON DELETE SET NULL,
    resolution          VARCHAR(24)
        CONSTRAINT ck_version_resolution CHECK (resolution IS NULL OR resolution IN
            ('FAST_FORWARD', 'LWW_WINNER', 'LWW_LOSER', 'IDENTICAL', 'INITIAL')),

    superseded_at       TIMESTAMPTZ,
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_version_slot_version UNIQUE (slot_id, version)
);

COMMENT ON COLUMN save_version.client_saved_at IS 'Untrusted client clock; LWW tiebreak only, rejected beyond max-clock-skew.';
COMMENT ON COLUMN save_version.conflict_lost_to IS 'Set on the losing side of an LWW resolution so the blob stays restorable.';

CREATE INDEX ix_version_slot     ON save_version (slot_id, version DESC);
CREATE INDEX ix_version_created  ON save_version (created_at);
-- Pruning job: find prunable (superseded, not a conflict loser) versions fast.
CREATE INDEX ix_version_prunable ON save_version (slot_id, created_at)
    WHERE superseded_at IS NOT NULL AND conflict_lost_to IS NULL;


-- =============================================================================
-- Codex progress (02_HISTORY_LAYER.md).
--
-- ~180 entries across 8 categories. Unlocking is diegetic: the player OBSERVEs
-- a tent's smoke hole, USEs a bow in the rain, asks a caravan master. Progress
-- must survive a platform switch — a codex unlocked on PC is unlocked on PS5.
--
-- Sync model: this is a grow-only set per player (an entry is never re-locked),
-- which makes the merge trivially conflict-free — a union. `read_count` and
-- `bookmarked` are mutable and use last-write-wins on `revision`.
-- =============================================================================

CREATE TABLE codex_progress (
    id              UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id       UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,

    -- e.g. CDX_KAYI_TRIBE. Content itself lives in the CDN JSON, not here:
    -- live-ops ships new entries without a client patch.
    codex_id        VARCHAR(64)  NOT NULL
        CONSTRAINT ck_codex_id_format CHECK (codex_id ~ '^CDX_[A-Z0-9_]{2,48}$'),

    -- Denormalised from the content pack so analytics can slice by confidence
    -- without joining the CDN catalogue. DOCUMENTED / DISPUTED / LEGEND.
    confidence      VARCHAR(12)  NOT NULL
        CONSTRAINT ck_codex_confidence CHECK (confidence IN ('DOCUMENTED', 'DISPUTED', 'LEGEND')),

    category        VARCHAR(16)  NOT NULL
        CONSTRAINT ck_codex_category CHECK (category IN
            ('PERSONS', 'PLACES', 'WARFARE', 'SOCIETY', 'ECONOMY', 'RELIGION', 'DAILY_LIFE', 'EVENTS')),

    unlock_method   VARCHAR(12)  NOT NULL
        CONSTRAINT ck_codex_unlock CHECK (unlock_method IN ('OBSERVE', 'USE', 'DIALOGUE', 'FIND', 'EVENT')),

    -- Where the player was when it unlocked. Feeds "which episodes teach the
    -- most history" analytics.
    episode_id      CHAR(5)
        CONSTRAINT ck_codex_episode CHECK (episode_id IS NULL OR episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),

    unlocked_at     TIMESTAMPTZ  NOT NULL,
    -- 0 = unlocked but never opened. The gap between unlocked and read is the
    -- single most important number for the history layer's effectiveness.
    read_count      INT          NOT NULL DEFAULT 0
        CONSTRAINT ck_codex_read CHECK (read_count >= 0),
    bookmarked      BOOLEAN      NOT NULL DEFAULT FALSE,

    -- Lamport-ish counter for LWW on the mutable fields.
    revision        BIGINT       NOT NULL DEFAULT 1,
    origin_device_id VARCHAR(64),
    updated_at      TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_codex_player_entry UNIQUE (player_id, codex_id)
);

COMMENT ON TABLE codex_progress IS 'Grow-only unlock set per player; union-merge across devices.';
COMMENT ON COLUMN codex_progress.read_count IS 'Unlocked-but-unread is the key funnel metric for the history layer.';

CREATE INDEX ix_codex_player      ON codex_progress (player_id);
CREATE INDEX ix_codex_updated     ON codex_progress (player_id, updated_at DESC);
-- Global rollups: "which codex entries does nobody ever read".
CREATE INDEX ix_codex_entry_stats ON codex_progress (codex_id, confidence);
