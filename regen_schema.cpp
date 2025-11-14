extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "access/xact.h"
#include "executor/spi.h"
#include "utils/builtins.h"
}

#include <fstream>
#include <string>

static const char *SCHEMA_PATH = "/var/lib/postgresql/pg_gen_query_schema.json";

static std::string get_schema_json()
{
  const char *query =
      "SELECT table_schema, table_name, column_name, data_type "
      "FROM information_schema.columns "
      "ORDER BY table_schema, table_name, ordinal_position";

  if (SPI_connect() != SPI_OK_CONNECT)
    elog(ERROR, "SPI_connect failed");

  int ret = SPI_execute(query, true, 0);
  if (ret != SPI_OK_SELECT)
    elog(ERROR, "SPI_execute failed");

  std::string result = "{ \"tables\": [";
  bool first_table = true;

  std::string cur_key;
  std::string cur_json;

  for (uint64 i = 0; i < SPI_processed; i++)
  {
    TupleDesc tupdesc = SPI_tuptable->tupdesc;
    HeapTuple tuple = SPI_tuptable->vals[i];

    char *schema = SPI_getvalue(tuple, tupdesc, 1);
    char *table = SPI_getvalue(tuple, tupdesc, 2);
    char *column = SPI_getvalue(tuple, tupdesc, 3);
    char *dtype = SPI_getvalue(tuple, tupdesc, 4);

    std::string key = std::string(schema) + "." + table;

    if (key != cur_key)
    {
      if (!cur_key.empty())
      {
        cur_json += "]}";
        if (!first_table)
          result += ",";
        result += cur_json;
        first_table = false;
      }

      cur_key = key;
      cur_json =
          "{ \"schema\": \"" + std::string(schema) +
          "\", \"table\": \"" + std::string(table) +
          "\", \"columns\": [";
    }
    else
    {
      cur_json += ",";
    }

    cur_json += "{ \"name\": \"" + std::string(column) +
                "\", \"type\": \"" + std::string(dtype) + "\"}";
  }

  if (!cur_key.empty())
  {
    cur_json += "]}";
    if (!first_table)
      result += ",";
    result += cur_json;
  }

  result += "] }";

  SPI_finish();
  return result;
}

extern "C"
{
  PG_FUNCTION_INFO_V1(regen_schema_cache);
  Datum regen_schema_cache(PG_FUNCTION_ARGS)
  {
    std::string json = get_schema_json();

    std::ofstream f(SCHEMA_PATH, std::ios::out | std::ios::trunc);
    if (!f.is_open())
      elog(ERROR, "Unable to write schema cache file: %s", SCHEMA_PATH);

    f << json;
    f.close();

    elog(LOG, "Schema cache refreshed: %s", SCHEMA_PATH);
    PG_RETURN_VOID();
  }
}