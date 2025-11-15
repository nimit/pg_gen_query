extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "utils/guc.h"
}

char *ai_openai_api_key = nullptr;
char *ai_anthropic_api_key = nullptr;

extern "C"
{
  void _PG_init(void)
  {
    DefineCustomStringVariable(
        "ai.openai_api_key",
        "OpenAI API key for pg_gen_query.",
        NULL,
        &ai_openai_api_key,
        NULL,
        PGC_SUSET,
        0,
        NULL, NULL, NULL);

    DefineCustomStringVariable(
        "ai.anthropic_api_key",
        "Anthropic API key for pg_gen_query.",
        NULL,
        &ai_anthropic_api_key,
        NULL,
        PGC_SUSET,
        0,
        NULL, NULL, NULL);
  }
}
