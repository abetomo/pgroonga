SRCS =						\
	src/pgrn-alias.c			\
	src/pgrn-auto-close.c			\
	src/pgrn-column-name.c			\
	src/pgrn-command-escape-value.c		\
	src/pgrn-convert.c			\
	src/pgrn-create.c			\
	src/pgrn-ctid.c				\
	src/pgrn-escape.c			\
	src/pgrn-flush.c			\
	src/pgrn-full-text-search-condition.c	\
	src/pgrn-global.c			\
	src/pgrn-groonga.c			\
	src/pgrn-groonga-tuple-is-alive.c	\
	src/pgrn-highlight-html.c		\
	src/pgrn-index-column-name.c		\
	src/pgrn-index-status.c			\
	src/pgrn-jsonb.c			\
	src/pgrn-keywords.c			\
	src/pgrn-match-positions-byte.c		\
	src/pgrn-match-positions-character.c	\
	src/pgrn-normalize.c			\
	src/pgrn-options.c			\
	src/pgrn-pg.c				\
	src/pgrn-query-escape.c			\
	src/pgrn-query-expand.c			\
	src/pgrn-query-extract-keywords.c	\
	src/pgrn-snippet-html.c			\
	src/pgrn-sequential-search.c		\
	src/pgrn-tokenize.c			\
	src/pgrn-vacuum.c			\
	src/pgrn-value.c			\
	src/pgrn-variables.c			\
	src/pgrn-wal.c				\
	src/pgrn-writable.c			\
	src/pgroonga.c

ifndef HAVE_XXHASH
SRCS +=						\
	vendor/xxHash/xxhash.c
endif
