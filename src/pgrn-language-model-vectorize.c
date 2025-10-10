#include "pgroonga.h"

#include <funcapi.h>
#include <utils/array.h>
#include <utils/builtins.h>

#include "pgrn-groonga.h"

PGDLLEXPORT PG_FUNCTION_INFO_V1(pgroonga_language_model_vectorize);

Datum
pgroonga_language_model_vectorize(PG_FUNCTION_ARGS)
{
	const char *tag = "planguage-model-vectorize]";
	text *modelName = PG_GETARG_TEXT_PP(0);
	text *target = PG_GETARG_TEXT_PP(1);

	grn_language_model_loader *loader = grn_language_model_loader_open(ctx);
	grn_language_model *model = NULL;
	grn_language_model_inferencer *inferencer = NULL;

	grn_language_model_loader_set_model(
		ctx, loader, text_to_cstring(modelName), VARSIZE_ANY_EXHDR(modelName));

	model = grn_language_model_loader_load(ctx, loader);
	if (!model)
	{
		grn_language_model_loader_close(ctx, loader);
		PGrnCheckRC(ctx->rc, "%s failed to load model: %s", tag, ctx->errbuf);
	}

	inferencer = grn_language_model_open_inferencer(ctx, model);
	if (!inferencer)
	{
		grn_language_model_close(ctx, model);
		grn_language_model_loader_close(ctx, loader);
		PGrnCheckRC(ctx->rc,
					"%s failed to open model inferencer: %s",
					tag,
					ctx->errbuf);
	}

	{
		Datum result;
		grn_obj vector;
		grn_rc rc;
		GRN_FLOAT32_INIT(&vector, GRN_OBJ_VECTOR);
		rc = grn_language_model_inferencer_vectorize(ctx,
													 inferencer,
													 text_to_cstring(target),
													 VARSIZE_ANY_EXHDR(target),
													 &vector);
		grn_language_model_inferencer_close(ctx, inferencer);
		grn_language_model_close(ctx, model);
		grn_language_model_loader_close(ctx, loader);

		if (rc != GRN_SUCCESS)
		{
			GRN_OBJ_FIN(ctx, &vector);
			PGrnCheckRC(
				ctx->rc, "%s failed to vectorize: %s", tag, ctx->errbuf);
		}

		result = PGrnConvertToDatum(&vector, FLOAT4ARRAYOID);
		GRN_OBJ_FIN(ctx, &vector);

		return result;
	}
}
