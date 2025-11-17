#include "schema_cache.h"

// Use pg shared memory (with lockless reads) as a cache (might result in better performance)
// Initial tests show accessing this results in a ~20% performance penalty (when compared to not reading schema at all)
// Need more testing
std::string schema_cache;

void clear_schema_cache()
{
  schema_cache.clear();
}
