CREATE FUNCTION pgroonga_tokenize(target text, VARIADIC options text[])
	RETURNS text[]
	AS 'MODULE_PATHNAME', 'pgroonga_tokenize'
	LANGUAGE C
	IMMUTABLE
	STRICT;

CREATE OR REPLACE FUNCTION pgroonga_escape(value float4)
	RETURNS text
	AS 'MODULE_PATHNAME', 'pgroonga_escape_float4'
	LANGUAGE C
	IMMUTABLE
	STRICT;
