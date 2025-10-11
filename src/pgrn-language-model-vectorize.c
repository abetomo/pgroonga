#include "pgrn-language-model-vectorize.h"
#include "pgroonga.h"

#include <funcapi.h>
#include <utils/array.h>
#include <utils/builtins.h>

#include <access/heapam.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <utils/acl.h>
#include <utils/builtins.h>

#include "pgrn-groonga.h"

static grn_language_model_loader *loader = NULL;
static grn_obj vector;

PGDLLEXPORT PG_FUNCTION_INFO_V1(pgroonga_language_model_vectorize);
PGDLLEXPORT PG_FUNCTION_INFO_V1(pgroonga_language_model_vectorizer);

typedef struct
{
	grn_language_model *model;
	grn_language_model_inferencer *inferencer;
} PGrnLanguageModelVectorizeData;

void
PGrnInitializeLanguageModelVectorize(void)
{
	loader = grn_language_model_loader_open(ctx);
	GRN_FLOAT32_INIT(&vector, GRN_OBJ_VECTOR);
}

void
PGrnFinalizeLanguageModelVectorize(void)
{
	GRN_OBJ_FIN(ctx, &vector);
	if (loader)
	{
		grn_language_model_loader_close(ctx, loader);
		loader = NULL;
	}
}

Datum
pgroonga_language_model_vectorize(PG_FUNCTION_ARGS)
{
	const char *tag = "[language-model-vectorize]";
	text *modelName = PG_GETARG_TEXT_PP(0);

	grn_language_model *model = NULL;
	grn_language_model_inferencer *inferencer = NULL;

	grn_language_model_loader_set_model(
		ctx, loader, text_to_cstring(modelName), VARSIZE_ANY_EXHDR(modelName));

	model = grn_language_model_loader_load(ctx, loader);
	if (!model)
	{
		PGrnCheckRC(ctx->rc, "%s failed to load model: %s", tag, ctx->errbuf);
	}

	inferencer = grn_language_model_open_inferencer(ctx, model);
	if (!inferencer)
	{
		grn_language_model_close(ctx, model);
		PGrnCheckRC(ctx->rc,
					"%s failed to open model inferencer: %s",
					tag,
					ctx->errbuf);
	}

	GRN_BULK_REWIND(&vector);
	{
		text *target = PG_GETARG_TEXT_PP(1);
		grn_rc rc =
			grn_language_model_inferencer_vectorize(ctx,
													inferencer,
													text_to_cstring(target),
													VARSIZE_ANY_EXHDR(target),
													&vector);
		grn_language_model_inferencer_close(ctx, inferencer);
		grn_language_model_close(ctx, model);

		if (rc != GRN_SUCCESS)
		{
			PGrnCheckRC(
				ctx->rc, "%s failed to vectorize: %s", tag, ctx->errbuf);
		}
	}

	return PGrnConvertToDatum(&vector, FLOAT4ARRAYOID);
}

Datum
pgroonga_language_model_vectorizer(PG_FUNCTION_ARGS)
{
	const char *tag = "[language-model-vectorizer]";
	FuncCallContext *context;
	PGrnLanguageModelVectorizeData *data;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		char *table = TextDatumGetCString(PG_GETARG_DATUM(0));
		char *column = TextDatumGetCString(PG_GETARG_DATUM(1));
		text *modelName = PG_GETARG_TEXT_PP(2);
		StringInfoData buf;
		int ret;

		/* Allocate function context to store data */
		context = SRF_FIRSTCALL_INIT();

		/* Initialize SPI manager */
		if ((ret = SPI_connect()) != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect failed: %d", ret);

		/* Build dynamic query */
		initStringInfo(&buf);
		appendStringInfo(&buf, "SELECT %s FROM %s", column, table);

		/* Execute query */
		ret = SPI_execute(buf.data, true, 0);
		if (ret != SPI_OK_SELECT)
			elog(ERROR, "SPI_execute failed: %d", ret);

		// language model
		data = palloc(sizeof(PGrnLanguageModelVectorizeData));
		grn_language_model_loader_set_model(ctx,
											loader,
											text_to_cstring(modelName),
											VARSIZE_ANY_EXHDR(modelName));

		data->model = grn_language_model_loader_load(ctx, loader);
		if (!data->model)
		{
			PGrnCheckRC(
				ctx->rc, "%s failed to load model: %s", tag, ctx->errbuf);
		}

		data->inferencer = grn_language_model_open_inferencer(ctx, data->model);
		if (!data->inferencer)
		{
			grn_language_model_close(ctx, data->model);
			PGrnCheckRC(ctx->rc,
						"%s failed to open model inferencer: %s",
						tag,
						ctx->errbuf);
		}

		oldcontext = MemoryContextSwitchTo(context->multi_call_memory_ctx);
		context->user_fctx = SPI_tuptable;
		context->max_calls = SPI_processed;
		context->tuple_desc = SPI_tuptable->tupdesc;
		context->user_fctx = data;
		MemoryContextSwitchTo(oldcontext);
	}

	context = SRF_PERCALL_SETUP();
	data = context->user_fctx;

	if (context->call_cntr < context->max_calls)
	{
		char *str = SPI_getvalue(
			SPI_tuptable->vals[context->call_cntr], context->tuple_desc, 1);

		GRN_BULK_REWIND(&vector);
		grn_rc rc = grn_language_model_inferencer_vectorize(
			ctx, data->inferencer, str, strlen(str), &vector);

		pfree(str);
		if (rc != GRN_SUCCESS)
		{
			PGrnCheckRC(
				ctx->rc, "%s failed to vectorize: %s", tag, ctx->errbuf);
		}
		SRF_RETURN_NEXT(context, PGrnConvertToDatum(&vector, FLOAT4ARRAYOID));
	}
	else
	{
		SPI_finish();
		grn_language_model_inferencer_close(ctx, data->inferencer);
		grn_language_model_close(ctx, data->model);
		pfree(data);
		SRF_RETURN_DONE(context);
	}
}
