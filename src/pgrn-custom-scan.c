#include <postgres.h>

#include <access/skey.h>
#include <nodes/extensible.h>
#include <optimizer/pathnode.h>
#include <optimizer/paths.h>
#include <optimizer/restrictinfo.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>

#include "pgroonga.h"

#include "pgrn-custom-scan.h"
#include "pgrn-groonga.h"
#include "pgrn-search.h"

typedef struct PGrnScanState
{
	CustomScanState parent; /* must be first field */
	grn_table_cursor *cursor;
	grn_obj *column; // todo
	Oid columTypeId; // todo
	PGrnSearchData data;
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
		// Only table scan is supported.
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
	cscan->scan.scanrelid = rel->relid;
	cscan->scan.plan.targetlist = tlist;
	cscan->scan.plan.qual = extract_actual_clauses(clauses, false);

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
PGrnLookupIndex(Relation rel, int errorLevel)
{
	ListCell *lc;
	List *indexes = RelationGetIndexList(rel);

	if (!rel)
	{
		return NULL;
	}

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

static void
PGrnBeginCustomScan(CustomScanState *node, EState *estate, int eflags)
{
	PGrnScanState *state = (PGrnScanState *) node;
	CustomScanState *customScanState = (CustomScanState *) state;
	TupleDesc tupdesc =
		RelationGetDescr(customScanState->ss.ss_currentRelation);
	Relation index;
	grn_obj *sourcesTable;
	List *quals = node->ss.ps.plan->qual;

	state->cursor = NULL;
	state->searched = NULL;

	index = PGrnLookupIndex(customScanState->ss.ss_currentRelation, ERROR);
	sourcesTable = PGrnLookupSourcesTable(index, ERROR);

	{
		ScanKeyData scankey;
		ListCell *lc;

		PGrnSearchDataInit(&(state->data), index, sourcesTable);
		foreach (lc, quals)
		{
			Expr *expr = (Expr *) lfirst(lc);
			OpExpr *opexpr;
			if (!IsA(expr, OpExpr))
			{
				continue;
			}

			opexpr = (OpExpr *) expr;
			if (list_length(opexpr->args) == 2)
			{
				Var *column = (Var *) linitial(opexpr->args);
				Const *value = (Const *) lsecond(opexpr->args);
				int strategy;
				Oid opFamily = index->rd_opfamily[0];
				Oid leftType;
				Oid rightType;

				get_op_opfamily_properties(opexpr->opno,
										   opFamily,
										   false,
										   &strategy,
										   &leftType,
										   &rightType);
				ScanKeyInit(&scankey,
							column->varattno,
							strategy,
							opexpr->opfuncid,
							value->constvalue);
				PGrnSearchBuildCondition(index, &scankey, &(state->data));
			}
		}
	}
	RelationClose(index);

	if (sourcesTable && grn_table_size(ctx, sourcesTable) > 0)
	{

		state->searched =
			grn_table_create(ctx,
							 NULL,
							 0,
							 NULL,
							 GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
							 sourcesTable,
							 0);
		if (!state->data.isEmptyCondition)
		{
			grn_table_selector *table_selector = grn_table_selector_open(
				ctx, sourcesTable, state->data.expression, GRN_OP_OR);
			grn_table_selector_select(ctx, table_selector, state->searched);
			grn_table_selector_close(ctx, table_selector);
		}

		state->cursor = grn_table_cursor_open(ctx,
											  state->searched,
											  NULL,
											  0,
											  NULL,
											  0,
											  0,
											  -1,
											  GRN_CURSOR_ASCENDING);
		if (state->cursor)
		{
			// todo
			Form_pg_attribute attr = TupleDescAttr(tupdesc, 0);
			state->column = PGrnLookupColumn(
				state->searched, NameStr(attr->attname), ERROR);
			state->columTypeId = attr->atttypid;
		}
	}
}

static TupleTableSlot *
PGrnExecCustomScan(CustomScanState *node)
{
	PGrnScanState *state = (PGrnScanState *) node;
	grn_id id;

	if (!state->cursor)
	{
		return NULL;
	}

	id = grn_table_cursor_next(ctx, state->cursor);
	if (!id)
	{
		return NULL;
	}

	{
		TupleTableSlot *slot = node->ss.ps.ps_ResultTupleSlot;
		grn_obj value;

		ExecClearTuple(slot);
		GRN_VOID_INIT(&value);
		grn_obj_get_value(ctx, state->column, id, &value);
		slot->tts_values[0] = PGrnConvertToDatum(&value, state->columTypeId);
		slot->tts_isnull[0] = false;
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
	PGrnSearchDataFree(&(state->data));
	if (state->searched)
	{
		grn_obj_close(ctx, state->searched);
		state->searched = NULL;
	}
	grn_table_cursor_close(ctx, state->cursor);
	grn_obj_unlink(ctx, state->column);
	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);
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
