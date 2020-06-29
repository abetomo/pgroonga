#pragma once

#ifdef WIN32
#	define PRId64 "I64d"
#	define PRIu64 "I64u"
#	define PGRN_PRIdSIZE "Id"
#	define PGRN_PRIuSIZE "Iu"
#else
#	include <inttypes.h>
#	define PGRN_PRIdSIZE "zd"
#	define PGRN_PRIuSIZE "zu"
#endif

#if PG_VERSION_NUM >= 90601
#	define PGRN_FUNCTION_INFO_V1(function_name)		\
	PGDLLEXPORT PG_FUNCTION_INFO_V1(function_name)
#else
#	define PGRN_FUNCTION_INFO_V1(function_name)				\
	extern PGDLLEXPORT PG_FUNCTION_INFO_V1(function_name)
#endif

#ifdef PGRN_HAVE_MSGPACK
#	define PGRN_SUPPORT_WAL
#endif

#if PG_VERSION_NUM < 110000
#	define PG_GETARG_JSONB_P(n) PG_GETARG_JSONB((n))
#	define DatumGetJsonbP(datum) DatumGetJsonb((datum))
#endif

#if PG_VERSION_NUM >= 110000
typedef const char *PGrnStringOptionValue;
#else
typedef char *PGrnStringOptionValue;
#endif

#if PG_VERSION_NUM >= 120000
#	define PGRN_SUPPORT_INDEX_CLAUSE
#	define PGRN_SUPPORT_TABLEAM
#	define PGRN_HAVE_OPTIMIZER_H
#endif

#if PG_VERSION_NUM >= 120000
#	define PGrnTableScanDesc TableScanDesc
#else
#	define PGrnTableScanDesc HeapScanDesc
#endif

#ifndef ERRCODE_SYSTEM_ERROR
#	define ERRCODE_SYSTEM_ERROR ERRCODE_IO_ERROR
#endif

#ifndef ALLOCSET_DEFAULT_SIZES
#	define ALLOCSET_DEFAULT_SIZES				\
	ALLOCSET_DEFAULT_MINSIZE,					\
	ALLOCSET_DEFAULT_INITSIZE,					\
	ALLOCSET_DEFAULT_MAXSIZE
#endif

#if PG_VERSION_NUM >= 100000
#	define PGRN_AM_INSERT_HAVE_INDEX_INFO
#	define PGRN_AM_COST_ESTIMATE_HAVE_INDEX_PAGES
#	define PGRN_INDEX_AM_ROUTINE_HAVE_AM_CAN_PARALLEL
#	define PGRN_INDEX_AM_ROUTINE_HAVE_AM_ESTIMATE_PARALLEL_SCAN
#	define PGRN_INDEX_AM_ROUTINE_HAVE_AM_INIT_PARALLEL_SCAN
#	define PGRN_INDEX_AM_ROUTINE_HAVE_AM_PARALLEL_RESCAN
#	define PGRN_SUPPORT_LOGICAL_REPLICATION
#endif

#define pgrn_array_create_iterator(array, slide_ndim)	\
	array_create_iterator(array, slide_ndim, NULL)

#if PG_VERSION_NUM >= 120000
#	define PGrnIndexBuildHeapScan(heap,				\
								  index,			\
								  indexInfo,		\
								  allowSync,		\
								  callback,			\
								  callbackState)	\
	table_index_build_scan((heap),					\
						   (index),					\
						   (indexInfo),				\
						   (allowSync),				\
						   false,					\
						   (callback),				\
						   (callbackState),			\
						   NULL)
#elif PG_VERSION_NUM >= 110000
#	define PGrnIndexBuildHeapScan(heap,				\
								  index,			\
								  indexInfo,		\
								  allowSync,		\
								  callback,			\
								  callbackState)	\
	IndexBuildHeapScan((heap),						\
					   (index),						\
					   (indexInfo),					\
					   (allowSync),					\
					   (callback),					\
					   (callbackState),				\
					   NULL)
#else
#	define PGrnIndexBuildHeapScan(heap,				\
								  index,			\
								  indexInfo,		\
								  allowSync,		\
								  callback,			\
								  callbackState)	\
	IndexBuildHeapScan((heap),						\
					   (index),						\
					   (indexInfo),					\
					   (allowSync),					\
					   (callback),					\
					   (callbackState))
#endif

#if PG_VERSION_NUM >= 120000
#	define PGRN_INDEX_SCAN_DESC_SET_FOUND_CTID(scan, ctid) \
	((scan)->xs_heaptid = (ctid))
#else
#	define PGRN_INDEX_SCAN_DESC_SET_FOUND_CTID(scan, ctid) \
	((scan)->xs_ctup.t_self = (ctid))
#endif

#if PG_VERSION_NUM >= 120000
#	define PGRN_RELATION_GET_RD_INDAM(relation) (relation)->rd_indam
#else
#	define PGRN_RELATION_GET_RD_INDAM(relation) (relation)->rd_amroutine
#endif

#ifdef PGRN_SUPPORT_TABLEAM
#	define pgrn_table_beginscan table_beginscan
#	define pgrn_table_beginscan_catalog table_beginscan_catalog
#else
#	define pgrn_table_beginscan heap_beginscan
#	define pgrn_table_beginscan_catalog heap_beginscan_catalog
#endif
