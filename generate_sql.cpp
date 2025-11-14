#include <string>
#include <stdexcept>

// Include your AI SDK headers here
// Example (adjust paths/namespaces to match your SDK structure):
// #include "aisdk/anthropic_client.h"

std::string generate_sql(const std::string &query)
{
  try
  {
    std::string full_prompt =
        "You are an expert SQL generator. "
        "Given a natural language description, return ONLY a SQL query.\n\n"
        "User request:\n" +
        query;

    // -----------------------------
    // Call AI model (replace with real implementation)
    // -----------------------------
    //
    // Example if using AnthropicClient — adjust to your real API:
    //
    // AnthropicClient client("<API_KEY>");
    // auto response = client.generate(full_prompt);
    // return response.text;

    // dummy
    std::string sql_query = "SELECT * FROM demo.products WHERE price > 20;";
    printf("GENERTED SQL QUERY: %s", sql_query.c_str());
    return sql_query;
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
