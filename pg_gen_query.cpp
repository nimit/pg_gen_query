extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
}

extern "C"
{
#ifdef PG_MODULE_MAGIC
  PG_MODULE_MAGIC;
#endif
}

#include <string>
#include <exception>

extern "C"
{
  Datum hello_cpp(PG_FUNCTION_ARGS);
  PG_FUNCTION_INFO_V1(hello_cpp);

  Datum hello_cpp(PG_FUNCTION_ARGS)
  {
    try
    {
      std::string s = "Hello from C++ extension!";
      text *t = cstring_to_text(s.c_str());
      PG_RETURN_TEXT_P(t);
    }
    catch (const std::exception &e)
    {
      /* Convert C++ exception to PostgreSQL error */
      ereport(ERROR,
              (errmsg("C++ exception in hello_cpp: %s", e.what())));
      PG_RETURN_NULL(); /* unreachable, but keeps compilers happy */
    }
    catch (...)
    {
      ereport(ERROR,
              (errmsg("Unknown C++ exception in hello_cpp")));
      PG_RETURN_NULL();
    }
  }
}