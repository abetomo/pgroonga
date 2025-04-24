/* Temporary test to be used during implementation of a custom scan. */

CREATE TABLE memos (content text);
CREATE INDEX memos_content ON memos USING pgroonga (content);
INSERT INTO memos VALUES ('PGroonga');
INSERT INTO memos VALUES ('Groonga');
INSERT INTO memos VALUES ('Mroonga');

SET pgroonga.enable_custom_scan = on;

EXPLAIN (COSTS OFF) SELECT * FROM memos where content &@ 'Groonga';
SELECT * FROM memos where content &@ 'Groonga';

DROP TABLE memos;
