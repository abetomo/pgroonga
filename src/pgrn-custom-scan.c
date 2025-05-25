#include <postgres.h>

#include <nodes/extensible.h>
#include <optimizer/pathnode.h>
#include <optimizer/paths.h>
#include <optimizer/restrictinfo.h>
#include <utils/lsyscache.h>

#include "pgroonga.h"

#include "pgrn-custom-scan.h"
#include "pgrn-groonga.h"
#include "pgrn-search.h"

typedef struct PGrnScanState
{
	CustomScanState parent; /* must be first field */
	grn_table_cursor *tableCursor;
	grn_obj columns;
	grn_obj *searched;
} PGrnScanState;

static bool PGrnCustomScanEnabled = false;
static set_rel_pathlist_hook_type PreviousSetRelPathlistHook = NULL;

static void PGrnSetRelPathlistHook(PlannerInfo *root,
								   RelOptInfo *rel,
								   Index rti,
								   RangeTblEntry *rte);
static Plan *PGrnPlanCustomPath(PlannerInfo *root,
								RelOptInfo *rel,
								struct CustomPath *best_path,
								List *tlist,
								List *clauses,
								List *custom_plans);
static List *PGrnReparameterizeCustomPathByChild(PlannerInfo *root,
												 List *custom_private,
												 RelOptInfo *child_rel);
static Node *PGrnCreateCustomScanState(CustomScan *cscan);
static void
PGrnBeginCustomScan(CustomScanState *node, EState *estate, int eflags);
static TupleTableSlot *PGrnExecCustomScan(CustomScanState *node);
static void PGrnEndCustomScan(CustomScanState *node);
static void PGrnReScanCustomScan(CustomScanState *node);
static void
PGrnExplainCustomScan(CustomScanState *node, List *ancestors, ExplainState *es);

const struct CustomPathMethods PGrnPathMethods = {
	.CustomName = "PGroongaScan",
	.PlanCustomPath = PGrnPlanCustomPath,
	.ReparameterizeCustomPathByChild = PGrnReparameterizeCustomPathByChild,
};

const struct CustomScanMethods PGrnScanMethods = {
	.CustomName = "PGroongaScan",
	.CreateCustomScanState = PGrnCreateCustomScanState,
};

const struct CustomExecMethods PGrnExecuteMethods = {
	.CustomName = "PGroongaScan",

	.BeginCustomScan = PGrnBeginCustomScan,
	.ExecCustomScan = PGrnExecCustomScan,
	.EndCustomScan = PGrnEndCustomScan,
	.ReScanCustomScan = PGrnReScanCustomScan,

	.ExplainCustomScan = PGrnExplainCustomScan,
};

bool
PGrnCustomScanGetEnabled(void)
{
	return PGrnCustomScanEnabled;
}

void
PGrnCustomScanEnable(void)
{
	PGrnCustomScanEnabled = true;
}

void
PGrnCustomScanDisable(void)
{
	PGrnCustomScanEnabled = false;
}

static void
PGrnSetRelPathlistHook(PlannerInfo *root,
					   RelOptInfo *rel,
					   Index rti,
					   RangeTblEntry *rte)
{
	CustomPath *cpath;
	if (PreviousSetRelPathlistHook)
	{
		PreviousSetRelPathlistHook(root, rel, rti, rte);
	}

	if (!PGrnCustomScanGetEnabled())
	{
		return;
	}

	if (get_rel_relkind(rte->relid) != RELKIND_RELATION)
	{
		// First, support table scan.
		return;
	}

	cpath = makeNode(CustomPath);
	cpath->path.pathtype = T_CustomScan;
	cpath->path.parent = rel;
	cpath->path.pathtarget = rel->reltarget;
	cpath->methods = &PGrnPathMethods;

	add_path(rel, &cpath->path);
}

static Plan *
PGrnPlanCustomPath(PlannerInfo *root,
				   RelOptInfo *rel,
				   struct CustomPath *best_path,
				   List *tlist,
				   List *clauses,
				   List *custom_plans)
{
	CustomScan *cscan = makeNode(CustomScan);
	cscan->methods = &PGrnScanMethods;
	cscan->scan.plan.qual = extract_actual_clauses(clauses, false);
	cscan->scan.plan.targetlist = tlist;
	cscan->scan.scanrelid = rel->relid;

	return &(cscan->scan.plan);
}

static List *
PGrnReparameterizeCustomPathByChild(PlannerInfo *root,
									List *custom_private,
									RelOptInfo *child_rel)
{
	return NIL;
}

static Node *
PGrnCreateCustomScanState(CustomScan *cscan)
{
	PGrnScanState *state =
		(PGrnScanState *) newNode(sizeof(PGrnScanState), T_CustomScanState);

	state->parent.methods = &PGrnExecuteMethods;

	return (Node *) &(state->parent);
}

static Relation
PGrnChooseIndex(Relation table_rel, int errorLevel)
{
	// todo: Support pgroonga_condition() index specification.
	// todo: Implementation of the logic for choosing which index to use.
	ListCell *lc;
	List *indexes;

	if (!table_rel)
		return NULL;

	indexes = RelationGetIndexList(table_rel);
	foreach (lc, indexes)
	{
		Oid indexId = lfirst_oid(lc);

		Relation index = RelationIdGetRelation(indexId);
		if (!PGrnIndexIsPGroonga(index))
		{
			RelationClose(index);
			continue;
		}
		return index;
	}
	return NULL;
}

static bool
PGrnOpInOpfamily(OpExpr *opexpr, Relation index)
{
	for (unsigned int i = 0; i < index->rd_att->natts; i++)
	{
		Oid opfamily = index->rd_opfamily[i];
		if (op_in_opfamily(opexpr->opno, opfamily))
		{
			return true;
		}
	}
	return false;
}

static void
PGrnSearchBuildCustomScanConditions(CustomScanState *customScanState,
									Relation index,
									PGrnSearchData *data)
{
	List *quals = customScanState->ss.ps.plan->qual;
	ListCell *cell;
	foreach (cell, quals)
	{
		Expr *expr = (Expr *) lfirst(cell);
		OpExpr *opexpr;
		Node *left;
		Node *right;
		Var *column;
		Const *value;

		if (!IsA(expr, OpExpr))
			continue;

		opexpr = (OpExpr *) expr;
		if (list_length(opexpr->args) != 2)
			continue;

		if (!PGrnOpInOpfamily(opexpr, index))
			continue;

		left = linitial(opexpr->args);
		right = lsecond(opexpr->args);

		if (nodeTag(left) == T_Var && nodeTag(right) == T_Const)
		{
			column = (Var *) left;
			value = (Const *) right;
		}
		else if (nodeTag(left) == T_Const && nodeTag(right) == T_Var)
		{
			column = (Var *) right;
			value = (Const *) left;
		}
		else
		{
			continue;
		}

		for (unsigned int i = 0; i < index->rd_att->natts; i++)
		{
			Oid opfamily = index->rd_opfamily[i];
			int strategy;
			Oid leftType;
			Oid rightType;
			ScanKeyData scankey;

			get_op_opfamily_properties(opexpr->opno,
									   opfamily,
									   false,
									   &strategy,
									   &leftType,
									   &rightType);
			ScanKeyInit(&scankey,
						column->varattno,
						strategy,
						opexpr->opfuncid,
						value->constvalue);
			PGrnSearchBuildCondition(index, &scankey, data);
		}
	}
}

static void
PGrnBeginCustomScan(CustomScanState *node, EState *estate, int eflags)
{
	PGrnScanState *state = (PGrnScanState *) node;
	CustomScanState *customScanState = (CustomScanState *) state;
	TupleDesc tupdesc =
		RelationGetDescr(customScanState->ss.ss_currentRelation);
	Relation index =
		PGrnChooseIndex(customScanState->ss.ss_currentRelation, ERROR);
	grn_obj *sourcesTable = PGrnLookupSourcesTable(index, ERROR);
	PGrnSearchData searchData;

	GRN_PTR_INIT(&(state->columns), GRN_OBJ_VECTOR, GRN_ID_NIL);
	for (int i = 0; i < tupdesc->natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
		grn_obj *column =
			PGrnLookupColumn(sourcesTable, NameStr(attr->attname), ERROR);
		GRN_PTR_PUT(ctx, &(state->columns), column);
	}

	PGrnSearchDataInit(&searchData, index, sourcesTable);
	PGrnSearchBuildCustomScanConditions(node, index, &searchData);

	if (!searchData.isEmptyCondition)
	{
		grn_table_selector *table_selector = grn_table_selector_open(
			ctx, sourcesTable, searchData.expression, GRN_OP_OR);

		state->searched =
			grn_table_create(ctx,
							 NULL,
							 0,
							 NULL,
							 GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
							 sourcesTable,
							 0);
		grn_table_selector_select(ctx, table_selector, state->searched);
		grn_table_selector_close(ctx, table_selector);
		state->tableCursor = grn_table_cursor_open(ctx,
												   state->searched,
												   NULL,
												   0,
												   NULL,
												   0,
												   0,
												   -1,
												   GRN_CURSOR_ASCENDING);
	}

	RelationClose(index);
	PGrnSearchDataFree(&searchData);
}

static TupleTableSlot *
PGrnExecCustomScan(CustomScanState *node)
{
	PGrnScanState *state = (PGrnScanState *) node;
	int i = 0;
	grn_id id;

	if (!state->tableCursor)
		return NULL;

	id = grn_table_cursor_next(ctx, state->tableCursor);
	if (!id)
		return NULL;

	{
		TupleTableSlot *slot = node->ss.ps.ps_ResultTupleSlot;
		grn_obj value;

		ExecClearTuple(slot);
		GRN_VOID_INIT(&value);
		for (i = 0; i < slot->tts_tupleDescriptor->natts; i++)
		{
			Form_pg_attribute attr = &(slot->tts_tupleDescriptor->attrs[i]);
			grn_obj *column = GRN_PTR_VALUE_AT(&(state->columns), i);
			grn_obj_get_value(ctx, column, id, &value);
			slot->tts_values[i] = PGrnConvertToDatum(&value, attr->atttypid);
			slot->tts_isnull[i] = false;
		}
		GRN_OBJ_FIN(ctx, &value);
		return ExecStoreVirtualTuple(slot);
	}
}

static void
PGrnExplainCustomScan(CustomScanState *node, List *ancestors, ExplainState *es)
{
	ExplainPropertyText("PGroongaScan", "DEBUG", es);
}

static void
PGrnEndCustomScan(CustomScanState *node)
{
	PGrnScanState *state = (PGrnScanState *) node;
	unsigned int i;
	unsigned int nTargetColumns;

	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);

	grn_table_cursor_close(ctx, state->tableCursor);
	nTargetColumns = GRN_BULK_VSIZE(&(state->columns)) / sizeof(grn_obj *);
	for (i = 0; i < nTargetColumns; i++)
	{
		grn_obj *column = GRN_PTR_VALUE_AT(&(state->columns), i);
		grn_obj_unlink(ctx, column);
	}
	GRN_OBJ_FIN(ctx, &(state->columns));

	if (state->searched)
	{
		grn_obj_close(ctx, state->searched);
		state->searched = NULL;
	}
}

static void
PGrnReScanCustomScan(CustomScanState *node)
{
}

void
PGrnInitializeCustomScan(void)
{
	PreviousSetRelPathlistHook = set_rel_pathlist_hook;
	set_rel_pathlist_hook = PGrnSetRelPathlistHook;

	RegisterCustomScanMethods(&PGrnScanMethods);
}

void
PGrnFinalizeCustomScan(void)
{
	set_rel_pathlist_hook = PreviousSetRelPathlistHook;
	// todo
	// Disable registered functions.
	// See also
	// https://github.com/pgroonga/pgroonga/pull/722#discussion_r2011284556
}
