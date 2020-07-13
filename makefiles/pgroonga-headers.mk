HEADERS =					\
	src/pgrn-alias.h			\
	src/pgrn-auto-close.h			\
	src/pgrn-column-name.h			\
	src/pgrn-command-escape-value.h		\
	src/pgrn-compatible.h			\
	src/pgrn-convert.h			\
	src/pgrn-create.h			\
	src/pgrn-ctid.h				\
	src/pgrn-full-text-search-condition.h	\
	src/pgrn-database.h			\
	src/pgrn-global.h			\
	src/pgrn-groonga-tuple-is-alive.h	\
	src/pgrn-groonga.h			\
	src/pgrn-highlight-html.h		\
	src/pgrn-index-status.h			\
	src/pgrn-jsonb.h			\
	src/pgrn-keywords.h			\
	src/pgrn-match-positions-byte.h		\
	src/pgrn-match-positions-character.h	\
	src/pgrn-normalize.h			\
	src/pgrn-options.h			\
	src/pgrn-pg.h				\
	src/pgrn-portable.h			\
	src/pgrn-query-expand.h			\
	src/pgrn-query-extract-keywords.h	\
	src/pgrn-search.h			\
	src/pgrn-sequential-search.h		\
	src/pgrn-tokenize.h			\
	src/pgrn-value.h			\
	src/pgrn-variables.h			\
	src/pgrn-wal.h				\
	src/pgrn-writable.h			\
	src/pgroonga.h

ifndef HAVE_XXHASH
HEADERS +=					\
	vendor/xxHash/xxhash.h
endif
