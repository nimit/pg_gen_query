extern "C"
{
#include "postgres.h"
#include "utils/elog.h"
}

#include <fstream>
#include <string>
#include <stdexcept>
#include <ai/core.h>
#include <ai/openai.h>
#include <ai/anthropic.h>
#include "constants.h"

extern char *ai_openai_api_key;
extern char *ai_anthropic_api_key;

static std::string get_schema()
{
  std::ifstream f(SCHEMA_PATH);
  if (!f.good())
  {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string generate_sql(const std::string &query)
{
  try
  {
    get_schema();
    std::string full_prompt =
        "You are an expert SQL generator."
        "Given a database schema and a natural language query, "
        "return ONLY an SQL query satisying ALL the conditions.\n"
        "Schema: `" +
        get_schema() +
        "`\nQuery: " +
        query;

    elog(INFO, "FULL PROMPT: %s", full_prompt.c_str());

    const char *openai = (ai_openai_api_key && ai_openai_api_key[0])
                             ? ai_openai_api_key
                             : getenv("OPENAI_API_KEY");

    const char *anthropic = (ai_anthropic_api_key && ai_anthropic_api_key[0])
                                ? ai_anthropic_api_key
                                : getenv("ANTHROPIC_API_KEY");
    ai::Client client;
    ai::GenerateOptions options;
    if (openai)
    {
      client = ai::openai::create_client(openai);
      options.model = "gpt-5-nano-2025-08-07";
    }
    else if (anthropic)
    {
      client = ai::anthropic::create_client(anthropic);
      options.model = ai::anthropic::models::kClaudeSonnet45;
    }
    else
    {
      elog(ERROR, "No LLM provider API key is found. Restart postgres service with either OPENAI_API_KEY OR ANTHROPIC_API_KEY set");
    }

    options.prompt = full_prompt;
    auto response = client.generate_text(options);
    // elog(INFO, "response finish: %s", response.finishReasonToString().c_str());
    if (response.is_success())
    {
      return response.text;
    }

    elog(ERROR, "AI Error: %s", response.error_message().c_str());

    // dummy
    // std::string sql_query = "SELECT * FROM demo.products WHERE price > 20;";
    // elog(INFO, "GENERTED SQL QUERY: %s", sql_query.c_str());
    // return sql_query;
  }
  catch (const std::exception &e)
  {
    throw std::runtime_error(std::string("generate_sql() failed: ") + e.what());
  }
  catch (...)
  {
    throw std::runtime_error("generate_sql() failed with unknown error");
  }
}
