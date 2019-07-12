CREATE TABLE memos (
  id integer,
  content varchar(256)
);

INSERT INTO memos VALUES (1, 'PostgreSQL is a RDBMS.');
INSERT INTO memos VALUES (2, 'Groonga is fast full text search engine.');
INSERT INTO memos VALUES (3, 'PGroonga is a PostgreSQL extension that uses Groonga.');

CREATE INDEX pgroonga_index ON memos
  USING pgroonga (content pgroonga_varchar_regexp_ops_v2);

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

EXPLAIN (COSTS OFF)
SELECT id, content
  FROM memos
 WHERE content &~| ARRAY['.ull', 'db.']::varchar[];

SELECT id, content
  FROM memos
 WHERE content &~| ARRAY['.ull', 'db.']::varchar[];

DROP TABLE memos;
