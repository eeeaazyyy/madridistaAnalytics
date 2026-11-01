CREATE TABLE matches (
    id              INTEGER PRIMARY KEY,
    kickoff         INTEGER NOT NULL,
    competition_id  INTEGER NOT NULL,
    home_team_id    INTEGER NOT NULL,
    home_team_name  TEXT NOT NULL,
    away_team_id    INTEGER NOT NULL,
    away_team_name  TEXT NOT NULL,
    status          TEXT NOT NULL,
    home_score      INTEGER,
    away_score      INTEGER
);

CREATE TABLE goal_events (
    match_id        INTEGER NOT NULL REFERENCES matches (id) ON DELETE CASCADE,
    team_id         INTEGER NOT NULL,
    period          TEXT NOT NULL,
    clock_seconds   INTEGER NOT NULL,
    added_minutes   INTEGER NOT NULL,
    type            TEXT NOT NULL,
    scorer_id       INTEGER,
    scorer_name     TEXT
);

CREATE INDEX idx_goal_events_match ON goal_events (match_id);

CREATE TABLE match_summaries (
    match_id        INTEGER PRIMARY KEY REFERENCES matches (id) ON DELETE CASCADE,
    payload         BLOB NOT NULL,
    fetched_at      INTEGER NOT NULL
);