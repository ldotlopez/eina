--
-- Control schemas versions
--
CREATE TABLE schema_versions (
	schema VARCHAR(32) PRIMARY KEY,
	version INTERGER
);

--
-- Variables table
--
CREATE TABLE variables (
	key VARCHAR(256) PRIMARY KEY,
	value VARCHAR(1024)
);

--
-- Main table to store information about streams
--   select strftime('%Y-%m-%dT%H:%m:%S.000000Z',datetime('now', 'UTC'));
CREATE TABLE streams (
	sid INTEGER PRIMARY KEY AUTOINCREMENT,
	uri VARCHAR(1024) UNIQUE NOT NULL,
	timestamp TIMESTAMP NOT NULL,
	played TIMESTAMP DEFAULT 0,
	count INTEGER DEFAULT 0
);
CREATE INDEX streams_sid_idx ON streams(sid);

-- 
-- Store metadata
--
CREATE TABLE metadata (
	sid INTEGER NOT NULL,
	key VARCHAR(128),
	value VARCHAR(128),
	CONSTRAINT metadata_pk PRIMARY KEY(sid,key),
	CONSTRAINT metadata_sid_fk FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX metadata_key_idx ON metadata(key);

--
-- Playlist history
--
CREATE TABLE playlist_history(
	timestamp TIMESTAMP NOT NULL,
	sid INTEGER,
	CONSTRAINT playlist_history_pk PRIMARY KEY(timestamp,sid),
	CONSTRAINT playlist_history_sid_fk FOREIGN KEY(sid) REFERENCES streams(sid) ON DELETE CASCADE ON UPDATE CASCADE
);


-- 
-- Recently plugin
--
CREATE TABLE playlist_aliases (
	timestamp TIMESTAMP PRIMARY KEY,
	alias VARCHAR(128),
	CONSTRAINT playlist_aliases_timestamp_fk FOREIGN KEY(timestamp) REFERENCES playlist_history(timestamp) ON DELETE CASCADE ON UPDATE RESTRICT
);
CREATE INDEX playlist_aliases_timestamp_idx ON playlist_aliases(timestamp);

