extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "executor/spi.h"
}

extern "C"
{
#ifdef PG_MODULE_MAGIC
  PG_MODULE_MAGIC;
#endif
}

#include <string>
#include <exception>
#include "generate_sql.h"

// TODO: Add support to return records (maybe in a separate function?)
// TODO: Add support for multiple queries
extern "C"
{
  PG_FUNCTION_INFO_V1(pg_gen_query);

  Datum pg_gen_query(PG_FUNCTION_ARGS)
  {
    if (PG_ARGISNULL(0))
    {
      PG_RETURN_NULL();
    }
    // ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
    // if (rsinfo == nullptr || !IsA(rsinfo, ReturnSetInfo))
    // {
    //   ereport(ERROR, (errmsg("set-valued function called in context that cannot accept a set")));
    // }
    // rsinfo->returnMode = SFRM_Materialize;
    // rsinfo->setDesc = nullptr;
    // Tuplestorestate *tupstore = tuplestore_begin_heap(true, false, 1024);
    // rsinfo->setResult = tupstore;
    try
    {
      text *input_text = PG_GETARG_TEXT_PP(0);
      std::string input(VARDATA_ANY(input_text), VARSIZE_ANY_EXHDR(input_text));
      // std::string sql_query = "no-op";
      std::string sql_query = generate_sql(input);
      PG_RETURN_TEXT_P(cstring_to_text(sql_query.c_str()));
      // if (SPI_connect() != SPI_OK_CONNECT)
      // {
      //   elog(ERROR, "SPI_connect failed");
      // }
      // auto ret = SPI_execute(sql_query.c_str(), true, 0);
      // if (ret != SPI_OK_SELECT && ret != SPI_OK_INSERT && ret != SPI_OK_UPDATE)
      // {
      //   elog(WARNING, "SPI_execute returned unexpected code: %d", ret);
      // }

      // SPITupleTable *tuptable = SPI_tuptable;
      // TupleDesc tupdesc = tuptable->tupdesc;
      // rsinfo->setDesc = CreateTupleDescCopy(tupdesc);

      // for (uint64 i = 0; i < SPI_processed; i++)
      // {
      //   tuplestore_puttuple(tupstore, tuptable->vals[i]);
      // }

      // SPI_finish();
      // tuplestore_donestoring(tupstore);
      return (Datum)0;
    }
    catch (const std::exception &e)
    {
      ereport(ERROR, (errmsg("C++ exception in hepg_gen_queryllo_cpp: %s", e.what())));
      PG_RETURN_NULL();
    }
    catch (...)
    {
      ereport(ERROR,
              (errmsg("Unknown C++ exception in pg_gen_query")));
      PG_RETURN_NULL();
    }
  }
}