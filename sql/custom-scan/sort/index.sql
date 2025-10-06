CREATE TABLE memos (
  id integer,
  content text
);

INSERT INTO memos VALUES (1, 'PostgreSQL is a RDBMS.');
INSERT INTO memos VALUES (2, 'Groonga is fast full text search engine.');
INSERT INTO memos VALUES (3, 'PGroonga is a PostgreSQL extension that uses Groonga.');
INSERT INTO memos VALUES (4, 'Mroonga is a MySQL storage engine that uses Groonga.');
INSERT INTO memos VALUES (5, 'I like Groonga.');

CREATE INDEX pgrn_index ON memos USING pgroonga (id, content);

SET pgroonga.enable_custom_scan = on;

EXPLAIN (COSTS OFF)
SELECT id, content, pgroonga_score(tableoid, ctid)
  FROM memos
 WHERE content &@~ 'PGroonga OR Groonga'
 ORDER BY id;

SELECT id, content, pgroonga_score(tableoid, ctid)
  FROM memos
 WHERE content &@~ 'PGroonga OR Groonga'
 ORDER BY id;

DROP TABLE memos;
