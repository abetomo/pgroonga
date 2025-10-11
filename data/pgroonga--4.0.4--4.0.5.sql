-- Upgrade SQL

CREATE FUNCTION pgroonga_language_model_vectorize(model_name text, target text)
	RETURNS float4[]
	AS 'MODULE_PATHNAME', 'pgroonga_language_model_vectorize'
	LANGUAGE C
	STRICT
	PARALLEL SAFE;

CREATE FUNCTION pgroonga_language_model_vectorizer(table_name text, column_name text, model_name text)
    RETURNS SETOF float4[]
	AS 'MODULE_PATHNAME', 'pgroonga_language_model_vectorizer'
	LANGUAGE C
	STRICT
	PARALLEL SAFE;
