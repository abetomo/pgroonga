/*
 * IDENTIFICATION
 *	pgroonga.h
 */

#ifndef PGROONGA_H
#define PGROONGA_H

#include <postgres.h>
#include <fmgr.h>

#ifndef PGDLLEXPORT
#  define PGDLLEXPORT
#endif

/* Groonga strategy types */
#define GrnLessStrategyNumber			1	/* operator < */
#define GrnLessEqualStrategyNumber		2	/* operator <= */
#define GrnEqualStrategyNumber			3	/* operator = */
#define GrnGreaterEqualStrategyNumber	4	/* operator >= */
#define GrnGreaterStrategyNumber		5	/* operator > */
#define GrnNotEqualStrategyNumber		6	/* operator <> (! in Groonga) */
#define GrnContainStrategyNumber		7	/* operator %% (@ in Groonga) */
#define GrnQueryStrategyNumber			8	/* operator @@ (Groonga query) */

/* Groonga support functions */
#define GrnTypeOfProc					1
#define GrnGetValueProc					2

/* file and table names */
#define GrnDatabaseBasename				"grn"
#define GrnIDsTableNamePrefix			"IDs"
#define GrnIDsTableNameFormat			GrnIDsTableNamePrefix "%u"
#define GrnLexiconNameFormat			"Lexicon%u"
#define GrnIndexColumnName				"index"

/* in pgroonga.c */
extern void PGDLLEXPORT _PG_init(void);

extern Datum PGDLLEXPORT pgroonga_contain_text(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_contain_bpchar(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_match(PG_FUNCTION_ARGS);

extern Datum PGDLLEXPORT pgroonga_insert(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_beginscan(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_gettuple(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_getbitmap(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_rescan(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_endscan(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_build(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_buildempty(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_bulkdelete(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_vacuumcleanup(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_costestimate(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_options(PG_FUNCTION_ARGS);

/* in groonga_types.c */
int pgroonga_bpchar_size(const BpChar *bpchar);

extern Datum PGDLLEXPORT pgroonga_typeof(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_text(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_bpchar(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_bool(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_int2(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_int4(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_int8(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_float4(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_float8(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_timestamp(PG_FUNCTION_ARGS);
extern Datum PGDLLEXPORT pgroonga_get_timestamptz(PG_FUNCTION_ARGS);

#endif	/* PPGROONGA_H */
