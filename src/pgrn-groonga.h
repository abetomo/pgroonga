#pragma once

#include <postgres.h>
#include <utils/rel.h>

#include <groonga.h>

#ifndef GRN_VERSION_OR_LATER
#	define GRN_VERSION_OR_LATER(major, minor, micro) 0
#endif

extern bool PGrnIsLZ4Available;
extern bool PGrnIsZlibAvailable;
extern bool PGrnIsZstdAvailable;
extern bool PGrnIsTemporaryIndexSearchAvailable;

void PGrnInitializeGroongaInformation(void);

const char *PGrnInspect(grn_obj *object);
const char *PGrnInspectName(grn_obj *object);

int PGrnRCToPgErrorCode(grn_rc rc);
bool PGrnCheck(const char *format,
			   ...) GRN_ATTRIBUTE_PRINTF(1);
bool PGrnCheckRC(grn_rc rc,
				 const char *format,
				 ...) GRN_ATTRIBUTE_PRINTF(2);
bool PGrnCheckRCLevel(grn_rc rc,
					  int errorLevel,
					  const char *format,
					  ...) GRN_ATTRIBUTE_PRINTF(3);

grn_obj *PGrnLookup(const char *name, int errorLevel);
grn_obj *PGrnLookupWithSize(const char *name,
							size_t nameSize,
							int errorLevel);
grn_obj *PGrnLookupColumn(grn_obj *table, const char *name, int errorLevel);
grn_obj *PGrnLookupColumnWithSize(grn_obj *table,
								  const char *name,
								  size_t nameSize,
								  int errorLevel);
grn_obj *PGrnLookupSourcesTable(Relation index, int errorLevel);
grn_obj *PGrnLookupSourcesCtidColumn(Relation index, int errorLevel);
grn_obj *PGrnLookupLexicon(Relation index,
						   unsigned int nthAttribute,
						   int errorLevel);
grn_obj *PGrnLookupIndexColumn(Relation index,
							   unsigned int nthAttribute,
							   int errorLevel);

void PGrnFormatSourcesTableName(const char *indexName,
								char output[GRN_TABLE_MAX_KEY_SIZE]);

grn_obj *PGrnCreateTable(Relation index,
						 const char *name,
						 grn_table_flags flags,
						 grn_obj *type,
						 grn_obj *tokenizer,
						 grn_obj *normalizers,
						 grn_obj *tokenFilters);
grn_obj *PGrnCreateTableWithSize(Relation index,
								 const char *name,
								 size_t nameSize,
								 grn_table_flags flags,
								 grn_obj *type,
								 grn_obj *tokenizer,
								 grn_obj *normalizers,
								 grn_obj *tokenFilters);
grn_obj *PGrnCreateSimilarTemporaryLexicon(Relation index,
										   unsigned int nthAttribute,
										   int errorLevel);
grn_obj *PGrnCreateColumn(Relation index,
						  grn_obj *table,
						  const char*name,
						  grn_column_flags flags,
						  grn_obj *type);
grn_obj *PGrnCreateColumnWithSize(Relation index,
								  grn_obj *table,
								  const char*name,
								  size_t nameSize,
								  grn_column_flags flags,
								  grn_obj *type);

void PGrnIndexColumnClearSources(Relation index,
								 grn_obj *indexColumn);
void PGrnIndexColumnSetSource(Relation index,
							  grn_obj *indexColumn,
							  grn_obj *source);
void PGrnIndexColumnSetSourceIDs(Relation index,
								 grn_obj *indexColumn,
								 grn_obj *sourceIDs);

void PGrnRenameTable(Relation index,
					 grn_obj *table,
					 const char *newName);

void PGrnRemoveObject(const char *name);
void PGrnRemoveObjectWithSize(const char *name, size_t nameSize);

void PGrnRemoveColumns(grn_obj *table);

void PGrnFlushObject(grn_obj *object, bool recursive);

grn_id PGrnPGTypeToGrnType(Oid pgTypeID, unsigned char *flags);
Oid PGrnGrnTypeToPGType(grn_id typeID);
