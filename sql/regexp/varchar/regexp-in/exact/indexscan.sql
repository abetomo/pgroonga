CREATE TABLE memos (
  id integer,
  content varchar(256)
);

INSERT INTO memos VALUES (1, 'PostgreSQL');
INSERT INTO memos VALUES (2, 'Groonga');
INSERT INTO memos VALUES (3, 'PGroonga');

CREATE INDEX pgroonga_index ON memos
  USING pgroonga (content pgroonga_varchar_regexp_ops_v2);

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

EXPLAIN (COSTS OFF)
SELECT id, content
  FROM memos
 WHERE content &~| ARRAY['\Agroonga\z', '\Apostgresql\z']::varchar[];

SELECT id, content
  FROM memos
 WHERE content &~| ARRAY['\Agroonga\z', '\Apostgresql\z']::varchar[];

DROP TABLE memos;
