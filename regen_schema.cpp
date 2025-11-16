extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "access/xact.h"
#include "executor/spi.h"
#include "utils/builtins.h"
#include "catalog/pg_type.h"
}

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>
#include "constants.h"
#include "schema_cache.h"

// TODO: optimize the schema result with abbreviations to reduce token size (explain abbreviations in the system prompt)

using json = nlohmann::json;

/*
 Helper utility: safely get SPI string column value
 returns empty std::string if null
*/
static std::string spi_get_str(HeapTuple tuple, TupleDesc tupdesc, int colno)
{
  char *val = SPI_getvalue(tuple, tupdesc, colno);
  std::string s;
  if (val)
  {
    s = std::string(val);
    pfree(val);
  }
  return s;
}

/*
 Key type used in maps: "schema.table"
*/
static inline std::string table_key(const std::string &schema, const std::string &table)
{
  return schema + "." + table;
}

/*
 Run a query and ensure it's SPI_OK_SELECT; returns SPI_processed
*/
static uint64 run_select_or_throw(const char *query)
{
  int rc = SPI_execute(query, true, 0);
  if (rc != SPI_OK_SELECT)
  {
    elog(ERROR, "SPI_execute failed: rc=%d query=%s", rc, query);
  }
  return SPI_processed;
}

/*
  create_detailed_schema_json()
  - Produces fully structured JSON per table (Option B style)
*/
static std::string create_detailed_schema_json()
{
  elog(INFO, "Creating detailed schema cache...");

  if (SPI_connect() != SPI_OK_CONNECT)
  {
    elog(ERROR, "SPI_connect failed");
  }

  // 1) Columns (with nullability, default, ordinal_position, data_type)
  const char *columns_q =
      "SELECT table_schema, table_name, column_name, data_type, is_nullable, "
      "       column_default, ordinal_position "
      "FROM information_schema.columns "
      "WHERE table_schema NOT IN ('pg_catalog', 'information_schema', 'pg_toast') "
      "  AND table_schema NOT LIKE 'pg_%' "
      "ORDER BY table_schema, table_name, ordinal_position;";

  run_select_or_throw(columns_q);
  std::unordered_map<std::string, json> tables;

  TupleDesc tupdesc = SPI_tuptable->tupdesc;
  for (uint64 i = 0; i < SPI_processed; ++i)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];

    std::string schema = spi_get_str(tuple, tupdesc, 1);
    std::string table = spi_get_str(tuple, tupdesc, 2);
    std::string column = spi_get_str(tuple, tupdesc, 3);
    std::string dtype = spi_get_str(tuple, tupdesc, 4);
    std::string is_nullable = spi_get_str(tuple, tupdesc, 5);
    std::string column_default = spi_get_str(tuple, tupdesc, 6);
    std::string ordinal_pos = spi_get_str(tuple, tupdesc, 7);

    std::string key = table_key(schema, table);

    if (!tables.count(key))
    {
      json j;
      j["schema"] = schema;
      j["table"] = table;
      j["columns"] = json::array();
      j["primary_key"] = nullptr;
      j["unique_constraints"] = json::array();
      j["foreign_keys"] = json::array();
      j["checks"] = json::array();
      j["indexes"] = json::array();
      j["table_comment"] = nullptr; // NEW: placeholder
      tables.emplace(key, std::move(j));
    }

    json col;
    col["name"] = column;
    col["type"] = dtype;
    col["nullable"] = (is_nullable == "YES");
    if (!column_default.empty())
      col["default"] = column_default;
    col["ordinal_position"] = std::stoi(ordinal_pos.empty() ? "0" : ordinal_pos);
    col["comment"] = nullptr; // NEW: placeholder

    tables[key]["columns"].push_back(col);
  }

  // 2) Primary keys (use information_schema.table_constraints + key_column_usage)
  const char *pk_q =
      "SELECT kcu.table_schema, kcu.table_name, tc.constraint_name, kcu.column_name, kcu.ordinal_position "
      "FROM information_schema.table_constraints tc "
      "JOIN information_schema.key_column_usage kcu "
      "  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema AND tc.table_name = kcu.table_name "
      "WHERE tc.constraint_type = 'PRIMARY KEY' "
      "ORDER BY kcu.table_schema, kcu.table_name, tc.constraint_name, kcu.ordinal_position;";

  run_select_or_throw(pk_q);
  tupdesc = SPI_tuptable->tupdesc;
  std::unordered_map<std::string, json> pk_map; // key -> {name, columns}
  for (uint64 i = 0; i < SPI_processed; ++i)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];
    std::string schema = spi_get_str(tuple, tupdesc, 1);
    std::string table = spi_get_str(tuple, tupdesc, 2);
    std::string constraint_name = spi_get_str(tuple, tupdesc, 3);
    std::string column_name = spi_get_str(tuple, tupdesc, 4);

    std::string key = table_key(schema, table);
    if (!pk_map.count(key))
    {
      json pkj;
      pkj["name"] = constraint_name;
      pkj["columns"] = json::array();
      pk_map.emplace(key, pkj);
    }
    pk_map[key]["columns"].push_back(column_name);
  }
  // attach pks to tables
  for (auto &kv : pk_map)
  {
    if (tables.count(kv.first))
      tables[kv.first]["primary_key"] = kv.second;
  }

  // 3) Unique constraints
  const char *uniq_q =
      "SELECT tc.table_schema, tc.table_name, tc.constraint_name, kcu.column_name "
      "FROM information_schema.table_constraints tc "
      "JOIN information_schema.key_column_usage kcu "
      "  ON tc.constraint_name = kcu.constraint_name AND tc.table_schema = kcu.table_schema AND tc.table_name = kcu.table_name "
      "WHERE tc.constraint_type = 'UNIQUE' "
      "ORDER BY tc.table_schema, tc.table_name, tc.constraint_name, kcu.ordinal_position;";

  run_select_or_throw(uniq_q);
  tupdesc = SPI_tuptable->tupdesc;
  std::unordered_map<std::string, json> uniq_map;
  for (uint64 i = 0; i < SPI_processed; ++i)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];
    std::string schema = spi_get_str(tuple, tupdesc, 1);
    std::string table = spi_get_str(tuple, tupdesc, 2);
    std::string constraint_name = spi_get_str(tuple, tupdesc, 3);
    std::string column_name = spi_get_str(tuple, tupdesc, 4);

    std::string key = table_key(schema, table);
    if (!uniq_map.count(key))
    {
      uniq_map.emplace(key, json::array());
    }
    // find or create constraint object
    bool found = false;
    for (auto &cobj : uniq_map[key])
    {
      if (cobj["name"] == constraint_name)
      {
        cobj["columns"].push_back(column_name);
        found = true;
        break;
      }
    }
    if (!found)
    {
      json c;
      c["name"] = constraint_name;
      c["columns"] = json::array();
      c["columns"].push_back(column_name);
      uniq_map[key].push_back(c);
    }
  }
  for (auto &kv : uniq_map)
  {
    if (tables.count(kv.first))
      tables[kv.first]["unique_constraints"] = kv.second;
  }

  // 4) Foreign keys with actions (use information_schema.referential_constraints + key_column_usage + constraint_column_usage)
  const char *fk_q =
      "SELECT con.conname AS constraint_name, "
      "rel_ns.nspname AS table_schema, "
      "rel.relname AS table_name, "
      "att2.attname AS column_name, "
      "frel_ns.nspname AS foreign_table_schema, "
      "frel.relname AS foreign_table_name, "
      "att.attname AS foreign_column_name, "
      "CASE con.confupdtype WHEN 'a' THEN 'NO ACTION' WHEN 'r' THEN 'RESTRICT' WHEN 'c' THEN 'CASCADE' WHEN 'n' THEN 'SET NULL' WHEN 'd' THEN 'SET DEFAULT' END AS on_update, "
      "CASE con.confdeltype WHEN 'a' THEN 'NO ACTION' WHEN 'r' THEN 'RESTRICT' WHEN 'c' THEN 'CASCADE' WHEN 'n' THEN 'SET NULL' WHEN 'd' THEN 'SET DEFAULT' END AS on_delete "
      "FROM pg_constraint con "
      "JOIN pg_class rel ON rel.oid = con.conrelid "
      "JOIN pg_namespace rel_ns ON rel_ns.oid = rel.relnamespace "
      "JOIN pg_class frel ON frel.oid = con.confrelid "
      "JOIN pg_namespace frel_ns ON frel_ns.oid = frel.relnamespace "
      "JOIN unnest(con.conkey) WITH ORDINALITY AS cols(attnum, ord) ON true "
      "JOIN pg_attribute att2 ON att2.attrelid = rel.oid AND att2.attnum = cols.attnum "
      "JOIN unnest(con.confkey) WITH ORDINALITY AS fcols(attnum, ord) ON fcols.ord = cols.ord "
      "JOIN pg_attribute att ON att.attrelid = frel.oid AND att.attnum = fcols.attnum "
      "WHERE con.contype = 'f' "
      "ORDER BY table_schema, table_name, constraint_name, cols.ord;";

  run_select_or_throw(fk_q);
  tupdesc = SPI_tuptable->tupdesc;
  std::unordered_map<std::string, json> fk_map;
  for (uint64 i = 0; i < SPI_processed; ++i)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];
    std::string constraint_name = spi_get_str(tuple, tupdesc, 1);
    std::string schema = spi_get_str(tuple, tupdesc, 2);
    std::string table = spi_get_str(tuple, tupdesc, 3);
    std::string column_name = spi_get_str(tuple, tupdesc, 4);
    std::string ref_schema = spi_get_str(tuple, tupdesc, 5);
    std::string ref_table = spi_get_str(tuple, tupdesc, 6);
    std::string ref_col = spi_get_str(tuple, tupdesc, 7);
    std::string on_update = spi_get_str(tuple, tupdesc, 8);
    std::string on_delete = spi_get_str(tuple, tupdesc, 9);

    std::string key = table_key(schema, table);
    if (!fk_map.count(key))
      fk_map.emplace(key, json::array());

    // find existing fk entry with same name or create new
    bool found = false;
    for (auto &fk : fk_map[key])
    {
      if (fk["name"] == constraint_name)
      {
        // append column pair
        fk["columns"].push_back(column_name);
        fk["references"]["columns"].push_back(ref_col);
        found = true;
        break;
      }
    }
    if (!found)
    {
      json fk;
      fk["name"] = constraint_name;
      fk["columns"] = json::array();
      fk["columns"].push_back(column_name);
      fk["references"] = {
          {"schema", ref_schema},
          {"table", ref_table},
          {"columns", json::array({ref_col})}};
      if (!on_update.empty())
        fk["on_update"] = on_update;
      if (!on_delete.empty())
        fk["on_delete"] = on_delete;
      fk_map[key].push_back(fk);
    }
  }
  for (auto &kv : fk_map)
  {
    if (tables.count(kv.first))
      tables[kv.first]["foreign_keys"] = kv.second;
  }

  // 5) Check constraints (from pg_constraint)
  // We'll query using pg_catalog to get name and definition
  const char *checks_q =
      "SELECT ns.nspname AS table_schema, rel.relname AS table_name, con.conname AS constraint_name, pg_get_constraintdef(con.oid) AS definition "
      "FROM pg_constraint con "
      "JOIN pg_class rel ON rel.oid = con.conrelid "
      "JOIN pg_namespace ns ON ns.oid = rel.relnamespace "
      "WHERE con.contype = 'c' "
      "  AND ns.nspname NOT IN ('pg_catalog', 'information_schema') "
      "  AND ns.nspname NOT LIKE 'pg_%' "
      "ORDER BY ns.nspname, rel.relname, con.conname;";

  run_select_or_throw(checks_q);
  tupdesc = SPI_tuptable->tupdesc;
  std::unordered_map<std::string, json> checks_map;
  for (uint64 i = 0; i < SPI_processed; ++i)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];
    std::string schema = spi_get_str(tuple, tupdesc, 1);
    std::string table = spi_get_str(tuple, tupdesc, 2);
    std::string constraint_name = spi_get_str(tuple, tupdesc, 3);
    std::string definition = spi_get_str(tuple, tupdesc, 4);

    std::string key = table_key(schema, table);
    if (!checks_map.count(key))
      checks_map.emplace(key, json::array());
    json c;
    c["name"] = constraint_name;
    c["definition"] = definition;
    checks_map[key].push_back(c);
  }
  for (auto &kv : checks_map)
  {
    if (tables.count(kv.first))
      tables[kv.first]["checks"] = kv.second;
  }

  // 6) Indexes (pg_indexes provides indexdef text)
  const char *indexes_q =
      "SELECT schemaname, tablename, indexname, indexdef "
      "FROM pg_indexes "
      "WHERE schemaname NOT IN ('pg_catalog', 'information_schema') "
      "  AND schemaname NOT LIKE 'pg_%' "
      "ORDER BY schemaname, tablename, indexname;";

  run_select_or_throw(indexes_q);
  tupdesc = SPI_tuptable->tupdesc;
  std::unordered_map<std::string, json> idx_map;
  for (uint64 i = 0; i < SPI_processed; ++i)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];
    std::string schemaname = spi_get_str(tuple, tupdesc, 1);
    std::string tablename = spi_get_str(tuple, tupdesc, 2);
    std::string indexname = spi_get_str(tuple, tupdesc, 3);
    std::string indexdef = spi_get_str(tuple, tupdesc, 4);

    std::string key = table_key(schemaname, tablename);
    if (!idx_map.count(key))
      idx_map.emplace(key, json::array());

    // try to extract column list from indexdef (best-effort)
    // indexdef looks like: "CREATE INDEX idxname ON schema.table USING btree (col1, (lower(col2::text)))"
    // We'll not perfectly parse all expressions, but we can attempt to capture the (...) contents.
    std::string cols_str;
    size_t pos = indexdef.find('(');
    size_t pos2 = indexdef.rfind(')');
    if (pos != std::string::npos && pos2 != std::string::npos && pos2 > pos)
    {
      cols_str = indexdef.substr(pos + 1, pos2 - pos - 1);
    }

    json idx;
    idx["name"] = indexname;
    idx["definition"] = indexdef;
    if (!cols_str.empty())
    {
      // split on commas (simple)
      json colarr = json::array();
      std::istringstream ss(cols_str);
      std::string tok;
      while (std::getline(ss, tok, ','))
      {
        // trim spaces
        size_t a = tok.find_first_not_of(" \t\n\r");
        size_t b = tok.find_last_not_of(" \t\n\r");
        if (a != std::string::npos && b != std::string::npos && b >= a)
        {
          colarr.push_back(tok.substr(a, b - a + 1));
        }
        else
        {
          colarr.push_back(tok);
        }
      }
      idx["columns"] = colarr;
    }
    else
    {
      idx["columns"] = json::array();
    }
    idx_map[key].push_back(idx);
  }
  for (auto &kv : idx_map)
  {
    if (tables.count(kv.first))
      tables[kv.first]["indexes"] = kv.second;
  }

  // 7) NEW: Table + Column Comments (pg_description + pg_class)
  const char *comments_q =
      "SELECT n.nspname AS schema, c.relname AS table_name, "
      "       a.attname AS column_name, d.description "
      "FROM pg_description d "
      "JOIN pg_class c ON c.oid = d.objoid "
      "JOIN pg_namespace n ON n.oid = c.relnamespace "
      "LEFT JOIN pg_attribute a ON a.attrelid = c.oid AND a.attnum = d.objsubid "
      "WHERE n.nspname NOT IN ('pg_catalog','information_schema') "
      "  AND n.nspname NOT LIKE 'pg_%';";

  run_select_or_throw(comments_q);
  tupdesc = SPI_tuptable->tupdesc;

  for (uint64 i = 0; i < SPI_processed; i++)
  {
    HeapTuple tuple = SPI_tuptable->vals[i];

    std::string schema = spi_get_str(tuple, tupdesc, 1);
    std::string table = spi_get_str(tuple, tupdesc, 2);
    std::string column = spi_get_str(tuple, tupdesc, 3);
    std::string comment = spi_get_str(tuple, tupdesc, 4);

    std::string key = table_key(schema, table);

    if (!tables.count(key))
      continue;

    if (column.empty())
    {
      // Table-level comment
      if (!comment.empty())
        tables[key]["table_comment"] = comment;
    }
    else
    {
      // Column-level comment
      auto &cols = tables[key]["columns"];
      for (auto &c : cols)
      {
        if (c["name"] == column)
        {
          if (!comment.empty())
            c["comment"] = comment;
          break;
        }
      }
    }
  }

  // All assembled - convert to JSON array
  json out;
  out["tables"] = json::array();
  for (auto &kv : tables)
  {
    // option: sort columns by ordinal_position
    auto &tbl = kv.second;
    if (tbl.contains("columns"))
    {
      std::sort(tbl["columns"].begin(), tbl["columns"].end(),
                [](const json &a, const json &b)
                {
                  return a.value("ordinal_position", 0) < b.value("ordinal_position", 0);
                });
      // drop ordinal_position field from final output (it was internal); keep if you want it
      for (auto &c : tbl["columns"])
      {
        c.erase("ordinal_position");
      }
    }
    out["tables"].push_back(tbl);
  }

  SPI_finish();
  std::string s = out.dump();
  elog(DEBUG1, "Detailed schema JSON length=%zu", s.size());
  return s;
}

/*
  create_flat_schema_json()
  - Produces flattened per-column JSON (user requested style)
  - We'll call the detailed generator internally and transform it.
*/
static std::string create_flat_schema_json()
{
  elog(INFO, "Creating flat schema cache...");

  std::string detailed = create_detailed_schema_json();
  json det = json::parse(detailed);

  json out;
  out["tables"] = json::array();

  for (auto &tbl : det["tables"])
  {
    json flat;
    flat["schema"] = tbl.value("schema", "");
    flat["table"] = tbl.value("table", "");

    if (!tbl["table_comment"].is_null())
      flat["table_comment"] = tbl["table_comment"];

    flat["columns"] = json::array();

    flat["indexes"] = tbl.value("indexes", json::array());

    // Process columns
    for (auto &col : tbl["columns"])
    {
      json c;
      c["name"] = col.value("name", "");
      c["type"] = col.value("type", "");

      if (col.contains("default") && !col["default"].is_null())
        c["default"] = col["default"];

      if (!col.value("nullable", true))
        c["nullable"] = false;

      if (col.contains("comment") && !col["comment"].is_null())
        c["comment"] = col["comment"];

      if (tbl.contains("primary_key") && !tbl["primary_key"].is_null())
      {
        for (auto &pkcol : tbl["primary_key"]["columns"])
        {
          if (pkcol == col["name"])
          {
            c["primary_key"] = true;
            break;
          }
        }
      }

      // unique constraints
      bool is_unique = false;
      if (tbl.contains("unique_constraints"))
      {
        for (auto &uc : tbl["unique_constraints"])
        {
          for (auto &u : uc["columns"])
          {
            if (u == col["name"])
            {
              is_unique = true;
              break;
            }
          }
        }
      }
      if (is_unique)
        c["unique"] = true;

      std::vector<std::string> fks;
      if (tbl.contains("foreign_keys"))
      {
        std::string colName = col["name"].get<std::string>();
        for (auto &fk : tbl["foreign_keys"])
        {
          const auto &cols = fk["columns"].get<std::vector<std::string>>();
          if (std::find(cols.begin(), cols.end(), colName) != cols.end())
          {
            std::string ref =
                fk["references"]["schema"].get<std::string>() + "." +
                fk["references"]["table"].get<std::string>() + "." +
                colName;

            fks.push_back(ref);
          }
        }
      }
      if (!fks.empty())
        c["foreign_keys"] = fks;

      std::vector<std::string> checks;
      if (tbl.contains("checks"))
      {
        for (auto &chk : tbl["checks"])
        {
          if (chk["name"] == col["name"])
          {
            checks.push_back(chk["definition"]);
          }
        }
      }
      if (!checks.empty())
        c["checks"] = checks;

      flat["columns"].push_back(c);
    }

    out["tables"].push_back(flat);
  }
  return out.dump();
}

extern "C"
{
  PG_FUNCTION_INFO_V1(regen_schema_cache);
  Datum regen_schema_cache(PG_FUNCTION_ARGS)
  {
    std::string json = create_flat_schema_json();
    // std::string json = create_detailed_schema_json();
    std::ofstream f(SCHEMA_PATH, std::ios::out | std::ios::trunc);
    if (!f.is_open())
    {
      elog(ERROR, "Unable to write schema cache file: %s", SCHEMA_PATH);
    }

    schema_cache.clear();

    f << json;
    f.close();

    elog(INFO, "Schema file refreshed: %s", SCHEMA_PATH);
    PG_RETURN_VOID();
  }
}