#include <postgres.h>

#include <nodes/extensible.h>
#include <optimizer/pathnode.h>
#include <optimizer/paths.h>
#include <optimizer/restrictinfo.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>

#include "pgroonga.h"

#include "pgrn-custom-scan.h"
#include "pgrn-groonga.h"

typedef struct PGrnScanState
{
	CustomScanState parent; /* must be first field */
	grn_table_cursor *cursor;
	grn_obj *column; // todo
	Oid columTypeId; // todo
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

static grn_obj *
PGrnLookupSourcesTableFromRel(Relation rel, int errorLevel)
{
	ListCell *lc;
	List *indexes = RelationGetIndexList(rel);
	grn_obj *table;

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
		table = PGrnLookupSourcesTable(index, ERROR);
		RelationClose(index);
		break;
	}
	return table;
}

static void
PGrnBeginCustomScan(CustomScanState *node, EState *estate, int eflags)
{
	PGrnScanState *state = (PGrnScanState *) node;
	CustomScanState *customScanState = (CustomScanState *) state;
	TupleDesc tupdesc =
		RelationGetDescr(customScanState->ss.ss_currentRelation);
	grn_obj *sourceTable;

	state->cursor = NULL;
	sourceTable = PGrnLookupSourcesTableFromRel(
		customScanState->ss.ss_currentRelation, ERROR);

	if (sourceTable && grn_table_size(ctx, sourceTable) > 0)
	{
		state->cursor = grn_table_cursor_open(
			ctx, sourceTable, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_ASCENDING);
		if (state->cursor)
		{
			// todo
			Form_pg_attribute attr = TupleDescAttr(tupdesc, 0);
			state->column =
				PGrnLookupColumn(sourceTable, NameStr(attr->attname), ERROR);
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
