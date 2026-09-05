-- =============================================================================
-- V1 — Identity: players, devices, refresh tokens, store entitlements.
--
-- Design notes:
--   * The game is single-player. An "account" exists only so progress follows
--     the player across PC / PlayStation / Xbox. There is no social graph.
--   * First contact is ALWAYS a device-linked anonymous account: the player
--     presses "Play" and gets an identity without typing anything. Store
--     entitlements (Steam/PSN/Xbox) are linked later and are what makes the
--     account recoverable.
-- =============================================================================

CREATE TABLE player (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),

    -- Human-facing handle. Nullable: anonymous device accounts have none until
    -- the player links a store account or types one in.
    display_name        VARCHAR(48),

    -- Telemetry consent, driven by Settings -> Account & Cloud -> Telemetry
    -- (07_SETTINGS_HOTKEYS.md). OFF means we drop events at the ingest edge.
    telemetry_consent   VARCHAR(16)  NOT NULL DEFAULT 'FULL'
        CONSTRAINT ck_player_consent CHECK (telemetry_consent IN ('FULL', 'ANONYMOUS', 'OFF')),

    -- Cloud save can be disabled per player; we still keep the row.
    cloud_save_enabled  BOOLEAN      NOT NULL DEFAULT TRUE,
    codex_sync_enabled  BOOLEAN      NOT NULL DEFAULT TRUE,

    -- Preferred locale, e.g. 'uz-Latn', 'uz-Cyrl', 'tr', 'en', 'ar'.
    locale              VARCHAR(16)  NOT NULL DEFAULT 'uz-Latn',

    -- Anti-cheat score. Accumulated from failed client HMACs, impossible
    -- telemetry, and rate-limit abuse. Never auto-bans (single-player game);
    -- it only excludes the player from the aggregate choice stats.
    integrity_score     INT          NOT NULL DEFAULT 0
        CONSTRAINT ck_player_integrity CHECK (integrity_score >= 0),

    -- Soft-delete for GDPR. The erase job nulls PII, keeps the row so foreign
    -- keys and anonymised aggregates stay valid.
    status              VARCHAR(16)  NOT NULL DEFAULT 'ACTIVE'
        CONSTRAINT ck_player_status CHECK (status IN ('ACTIVE', 'SUSPENDED', 'ERASURE_PENDING', 'ERASED')),
    erasure_requested_at TIMESTAMPTZ,

    -- Optimistic lock: two devices may PATCH the profile concurrently (console
    -- changing locale while PC changes telemetry consent). Losing one silently
    -- would make a setting appear to revert by itself.
    version_lock        BIGINT       NOT NULL DEFAULT 0,

    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),
    last_seen_at        TIMESTAMPTZ
);

COMMENT ON TABLE  player IS 'One row per human. Meta-services only — no gameplay state lives here.';
COMMENT ON COLUMN player.integrity_score IS 'Anti-cheat suspicion score; excludes player from ChoiceAggregate above threshold.';

CREATE INDEX ix_player_last_seen ON player (last_seen_at DESC NULLS LAST);
CREATE INDEX ix_player_erasure   ON player (erasure_requested_at)
    WHERE status = 'ERASURE_PENDING';


-- -----------------------------------------------------------------------------
-- Devices. A player may play on several machines; each gets a stable device id
-- generated client-side. The device id is ALSO the vector-clock node id used by
-- the cloud-save conflict resolver, which is why it is a first-class row.
-- -----------------------------------------------------------------------------
CREATE TABLE player_device (
    id              UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id       UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,

    -- Opaque, client-generated, stable across reinstalls where the platform
    -- allows it. This string is the vector clock key.
    device_id       VARCHAR(64)  NOT NULL,

    platform        VARCHAR(16)  NOT NULL
        CONSTRAINT ck_device_platform CHECK (platform IN ('PC_STEAM', 'PC_EPIC', 'PS5', 'XBOX_SERIES', 'UNKNOWN')),
    device_label    VARCHAR(64),
    app_version     VARCHAR(32),

    created_at      TIMESTAMPTZ  NOT NULL DEFAULT now(),
    last_seen_at    TIMESTAMPTZ  NOT NULL DEFAULT now(),

    CONSTRAINT uq_device_id UNIQUE (device_id)
);

COMMENT ON COLUMN player_device.device_id IS 'Vector-clock node id. Uniqueness is global, not per-player.';

CREATE INDEX ix_device_player ON player_device (player_id);


-- -----------------------------------------------------------------------------
-- Refresh tokens. Access tokens are short-lived RS256 JWTs and are NOT stored.
-- Refresh tokens are stored hashed so a database leak cannot mint sessions.
-- Rotation: every refresh issues a new token and marks the old one used. A
-- reused token means theft -> the whole family is revoked.
-- -----------------------------------------------------------------------------
CREATE TABLE refresh_token (
    id              UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id       UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,
    device_id       VARCHAR(64)  NOT NULL,

    -- SHA-256 of the opaque token. Never store the token itself.
    token_hash      CHAR(64)     NOT NULL,

    -- Rotation family: all descendants of one login share this id.
    family_id       UUID         NOT NULL,

    issued_at       TIMESTAMPTZ  NOT NULL DEFAULT now(),
    expires_at      TIMESTAMPTZ  NOT NULL,
    used_at         TIMESTAMPTZ,
    revoked_at      TIMESTAMPTZ,
    revoked_reason  VARCHAR(48),

    CONSTRAINT uq_refresh_hash UNIQUE (token_hash)
);

CREATE INDEX ix_refresh_player  ON refresh_token (player_id);
CREATE INDEX ix_refresh_family  ON refresh_token (family_id);
-- Cleanup job scans by expiry; partial index keeps it small.
CREATE INDEX ix_refresh_expired ON refresh_token (expires_at)
    WHERE revoked_at IS NULL;


-- -----------------------------------------------------------------------------
-- Store entitlements. Verifying that the player actually owns the game on the
-- platform they claim. Linking an entitlement is what upgrades an anonymous
-- device account into a recoverable one.
-- -----------------------------------------------------------------------------
CREATE TABLE entitlement (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id           UUID         NOT NULL
        REFERENCES player (id) ON DELETE CASCADE,

    provider            VARCHAR(16)  NOT NULL
        CONSTRAINT ck_entitlement_provider CHECK (provider IN ('STEAM', 'PSN', 'XBOX', 'EPIC')),

    -- SteamID64 / PSN account id / XUID.
    provider_account_id VARCHAR(64)  NOT NULL,

    -- Which SKU: base game, season pass, deluxe.
    product_sku         VARCHAR(64)  NOT NULL DEFAULT 'DIRILIS_BASE',

    status              VARCHAR(16)  NOT NULL DEFAULT 'VALID'
        CONSTRAINT ck_entitlement_status CHECK (status IN ('VALID', 'REVOKED', 'REFUNDED', 'UNVERIFIED')),

    verified_at         TIMESTAMPTZ,
    -- Re-verified periodically: refunds and chargebacks revoke entitlements
    -- after the fact.
    revalidate_after    TIMESTAMPTZ,
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT now(),

    -- One store account maps to exactly one player per provider.
    CONSTRAINT uq_entitlement_provider_account UNIQUE (provider, provider_account_id, product_sku)
);

COMMENT ON TABLE entitlement IS 'Store ownership proof. Verification itself is a stub pending platform SDK keys.';

CREATE INDEX ix_entitlement_player      ON entitlement (player_id);
CREATE INDEX ix_entitlement_revalidate  ON entitlement (revalidate_after)
    WHERE status = 'VALID';
