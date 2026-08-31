-- =============================================================================
-- V4 — "Safar Daftari" (Journey Log) — 02_HISTORY_LAYER.md §4.
--
-- The player's personal, auto-written diary. The game composes entries in the
-- player's own voice from what actually happened: the choices made, the people
-- met, the codex entries unlocked. It is a retention AND a marketing feature —
-- a player who exports their diary to PDF and posts it is doing our marketing.
--
-- Why server-side: the diary must survive a platform switch, must be
-- exportable, and a share link must resolve for someone who does not own the
-- game. That rules out keeping it inside the save blob.
--
-- Text is stored as authored text, not loc keys: the entries are generated
-- from templates at runtime in the player's language, and the diary is
-- explicitly a personal record. Re-localising someone's diary on a language
-- switch would be wrong.
-- =============================================================================

CREATE TABLE journey_entry (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id           UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,

    -- Which run this belongs to. NG+ / a second playthrough writes a new diary
    -- rather than appending to the old one; the slot's save id anchors it.
    playthrough_id      UUID         NOT NULL,

    -- Ordering within the diary. Client-assigned, gap-tolerant, so an offline
    -- device can write entries without coordinating.
    sequence_no         INT          NOT NULL
        CONSTRAINT ck_journey_seq CHECK (sequence_no >= 0),

    episode_id          CHAR(5)      NOT NULL
        CONSTRAINT ck_journey_episode CHECK (episode_id ~ '^EP0(0[1-9]|[1-3][0-9]|4[0-8])$'),
    season_id           CHAR(2)      NOT NULL
        CONSTRAINT ck_journey_season CHECK (season_id ~ '^S[1-4]$'),

    -- Dual calendar (02_HISTORY_LAYER.md §5). Both are shown: "632 Rabi al-awwal
    -- / 1234 December". Stored as authored strings because the hijri rendering
    -- is a narrative choice, not a computed conversion.
    hijri_date_text     VARCHAR(64)  NOT NULL,
    gregorian_date_text VARCHAR(64)  NOT NULL,
    -- Sortable anchor for timeline rendering and PDF ordering.
    in_game_date        DATE         NOT NULL,

    place_name          VARCHAR(96),

    -- The diary text itself, in the player's voice.
    body                TEXT         NOT NULL
        CONSTRAINT ck_journey_body_len CHECK (char_length(body) BETWEEN 1 AND 8000),

    -- Codex entries referenced at the bottom of the page ("[Sultan Han]
    -- [Caravanserai system] [Dirham]"). Array, not a join table: it is read as
    -- a unit and never queried by element.
    linked_codex_ids    TEXT[]       NOT NULL DEFAULT '{}',

    -- Mood tint for the page, derived from world_state at write time
    -- (sabr/iman/hand phase). Drives the PDF's ink colour and border.
    tone                VARCHAR(16)  NOT NULL DEFAULT 'NEUTRAL'
        CONSTRAINT ck_journey_tone CHECK (tone IN ('NEUTRAL', 'HOPEFUL', 'GRIEVING', 'RESOLUTE', 'WOUNDED')),

    -- Entries written after EP024 render with an unsteady left-handed script in
    -- the PDF. A small touch, but it is the whole point of the wound system.
    written_left_handed BOOLEAN      NOT NULL DEFAULT FALSE,

    -- Player can hide an entry from a shared diary without deleting it.
    hidden_from_share   BOOLEAN      NOT NULL DEFAULT FALSE,

    origin_device_id    VARCHAR(64),
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),

    -- Idempotency: the client may retry a write after a network drop.
    CONSTRAINT uq_journey_playthrough_seq UNIQUE (playthrough_id, sequence_no)
);

COMMENT ON TABLE  journey_entry IS 'Auto-written personal diary. Exportable to PDF, shareable via public link.';
COMMENT ON COLUMN journey_entry.written_left_handed IS 'Post-EP024 entries render in an unsteady left-handed hand in the PDF.';
COMMENT ON COLUMN journey_entry.linked_codex_ids IS 'Denormalised codex references; read as a unit, never queried by element.';

CREATE INDEX ix_journey_player   ON journey_entry (player_id, playthrough_id, sequence_no);
CREATE INDEX ix_journey_episode  ON journey_entry (player_id, episode_id);
CREATE INDEX ix_journey_recent   ON journey_entry (player_id, created_at DESC);


-- -----------------------------------------------------------------------------
-- Public share links.
--
-- A share token resolves for anyone, including people who do not own the game
-- — that is the marketing value. So: unguessable token, revocable, expiring,
-- and it exposes ONLY diary text. No player id, no save data, no email.
-- -----------------------------------------------------------------------------
CREATE TABLE journey_share (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id           UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,
    playthrough_id      UUID         NOT NULL,

    -- URL-safe random, >= 128 bits of entropy. Unguessable is the only access
    -- control a public link can have.
    share_token         VARCHAR(64)  NOT NULL,

    -- Optional display name shown on the public page instead of anything real.
    public_title        VARCHAR(96),

    -- Range of the diary being shared. NULL end = "everything so far", which
    -- keeps growing as the player plays.
    from_sequence_no    INT,
    to_sequence_no      INT,

    view_count          BIGINT       NOT NULL DEFAULT 0,
    expires_at          TIMESTAMPTZ  NOT NULL,
    revoked_at          TIMESTAMPTZ,
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_journey_share_token UNIQUE (share_token),
    CONSTRAINT ck_journey_share_range CHECK (
        from_sequence_no IS NULL OR to_sequence_no IS NULL OR to_sequence_no >= from_sequence_no
    )
);

COMMENT ON TABLE journey_share IS 'Public, revocable, expiring link. Exposes diary text only — never player identity.';

CREATE INDEX ix_share_player ON journey_share (player_id);
-- Resolution path is token -> row, filtered on liveness.
CREATE INDEX ix_share_live   ON journey_share (share_token)
    WHERE revoked_at IS NULL;


-- -----------------------------------------------------------------------------
-- PDF exports. Generation is async (a 48-episode diary is a real document), so
-- the request creates a PENDING row, a worker fills it, and the client polls.
-- -----------------------------------------------------------------------------
CREATE TABLE journey_export (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id           UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,
    playthrough_id      UUID         NOT NULL,

    format              VARCHAR(8)   NOT NULL DEFAULT 'PDF'
        CONSTRAINT ck_export_format CHECK (format IN ('PDF', 'PNG')),

    status              VARCHAR(16)  NOT NULL DEFAULT 'PENDING'
        CONSTRAINT ck_export_status CHECK (status IN ('PENDING', 'RUNNING', 'READY', 'FAILED', 'EXPIRED')),

    -- S3 key in the exports bucket once READY.
    object_key          VARCHAR(256),
    size_bytes          INT,
    entry_count         INT,

    failure_reason      VARCHAR(256),

    requested_at        TIMESTAMPTZ  NOT NULL DEFAULT now(),
    completed_at        TIMESTAMPTZ,
    -- Exports are cleaned up after ertugrul.journey.export-ttl.
    expires_at          TIMESTAMPTZ  NOT NULL
);

CREATE INDEX ix_export_player  ON journey_export (player_id, requested_at DESC);
CREATE INDEX ix_export_pending ON journey_export (requested_at)
    WHERE status IN ('PENDING', 'RUNNING');
CREATE INDEX ix_export_expiry  ON journey_export (expires_at)
    WHERE status = 'READY';
