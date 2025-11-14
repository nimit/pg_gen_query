extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "executor/spi.h"
}

#include <fstream>
#include <string>
#include <stdexcept>

static std::string schema_path = "/var/lib/postgresql/schema.json";
static std::string version_path = "/var/lib/postgresql/schema.version";

// -------- Utility: read entire file --------
static std::string read_file(const std::string &path)
{
  std::ifstream f(path);
  if (!f.good())
    return "";
  return std::string((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
}

static void write_file(const std::string &path, const std::string &data)
{
  std::ofstream f(path);
  f << data;
}

// -------- Compute DB schema checksum --------
static std::string compute_schema_version()
{
  if (SPI_connect() != SPI_OK_CONNECT)
    ereport(ERROR, (errmsg("SPI_connect failed")));

  const char *sql =
      "SELECT md5(string_agg(row, '')) FROM ("
      "  SELECT table_name || ':' || column_name || ':' || data_type AS row "
      "  FROM information_schema.columns "
      "  WHERE table_schema NOT IN ('pg_catalog','information_schema') "
      "  ORDER BY table_name, column_name"
      ") x";

  int ret = SPI_execute(sql, true, 1);
  if (ret != SPI_OK_SELECT)
    ereport(ERROR, (errmsg("schema checksum query failed")));

  Datum d = SPI_getbinval(SPI_tuptable->vals[0],
                          SPI_tuptable->tupdesc,
                          1,
                          nullptr);

  char *s = DatumGetCString(d);
  SPI_finish();
  return std::string(s);
}

// -------- Dump schema as JSON --------
static std::string dump_schema_json()
{
  if (SPI_connect() != SPI_OK_CONNECT)
    ereport(ERROR, (errmsg("SPI_connect failed")));

  const char *sql =
      "SELECT json_agg(obj) FROM ("
      "  SELECT table_name, "
      "         json_agg(json_build_object("
      "             'column', column_name, 'type', data_type"
      "         )) AS columns "
      "  FROM information_schema.columns "
      "  WHERE table_schema NOT IN ('pg_catalog', 'information_schema') "
      "  GROUP BY table_name"
      ") obj";

  int ret = SPI_execute(sql, true, 0);
  if (ret != SPI_OK_SELECT)
    ereport(ERROR, (errmsg("schema dump failed")));

  Datum d = SPI_getbinval(SPI_tuptable->vals[0],
                          SPI_tuptable->tupdesc,
                          1,
                          nullptr);

  char *s = DatumGetCString(d);
  SPI_finish();
  return std::string(s);
}

// -------- Public function: load schema --------
std::string load_schema_cached()
{
  std::string existing_version = read_file(version_path);
  std::string db_version = compute_schema_version();

  if (existing_version != db_version)
  {
    std::string schema_json = dump_schema_json();
    write_file(schema_path, schema_json);
    write_file(version_path, db_version);
    return schema_json;
  }

  return read_file(schema_path);
}
