CREATE TABLE memos (
  content text
);

INSERT INTO memos VALUES ('I am a king.');
INSERT INTO memos VALUES ('I am a queen.');

SELECT pgroonga_language_model_vectorizer[1:3]
  FROM pgroonga_language_model_vectorizer(
         'memos',
         'content',
         'hf:///groonga/bge-m3-Q4_K_M-GGUF');
