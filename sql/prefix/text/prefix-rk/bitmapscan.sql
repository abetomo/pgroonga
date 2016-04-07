CREATE TABLE readings (
  katakana text
);

INSERT INTO readings VALUES ('ポストグレスキューエル');
INSERT INTO readings VALUES ('グルンガ');
INSERT INTO readings VALUES ('ピージールンガ');
INSERT INTO readings VALUES ('ピージーロジカル');

CREATE INDEX pgrn_index ON readings
  USING pgroonga (katakana pgroonga.prefix_search_ops_v2);

SET enable_seqscan = off;
SET enable_indexscan = off;
SET enable_bitmapscan = on;

SELECT katakana
  FROM readings
 WHERE katakana &^~ 'p';

DROP TABLE readings;
