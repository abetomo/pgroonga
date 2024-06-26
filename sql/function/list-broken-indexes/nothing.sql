CREATE TABLE memos (
  title text,
  content text
);

CREATE TABLE tags (
  name text
);

CREATE INDEX pgrn_memos_index ON memos USING PGroonga (title, content);
CREATE INDEX pgrn_tags_index ON tags USING PGroonga (name);

SELECT * FROM pgroonga_list_broken_indexes();

DROP TABLE memos;
DROP TABLE tags;
