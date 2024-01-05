CREATE TABLE tags (
  id int,
  names varchar[]
);

INSERT INTO tags VALUES (1, ARRAY['PostgreSQL', 'PG']);
INSERT INTO tags VALUES (2, ARRAY['Groonga', 'grn']);
INSERT INTO tags VALUES (3, ARRAY['PGroonga', 'pgrn']);
INSERT INTO tags VALUES (4, ARRAY[]::varchar[]);

CREATE INDEX pgrn_index ON tags
  USING pgroonga (names)
  WITH (normalizers='NormalizerNFKC130("remove_symbol", true)');

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

\pset format unaligned
EXPLAIN (COSTS OFF)
SELECT names
  FROM tags
 WHERE names &^ pgroonga_condition('-p_G', index_name => 'pgrn_index')
 ORDER BY id
\g |sed -r -e "s/('.+'|ROW.+)::pgroonga/pgroonga/g"
\pset format aligned

SELECT names
  FROM tags
 WHERE names &^ pgroonga_condition('-p_G', index_name => 'pgrn_index')
 ORDER BY id;

DROP TABLE tags;
