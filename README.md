# pg_gen_query: Query PostgreSQL with Natural Language

This PostgreSQL extension provides a function, `pg_gen_query`, that converts natural language input into an equivalent SQL command.

![pg_gen_query_gif](https://github.com/user-attachments/assets/1e2dd00a-a679-467c-8ed4-11e99012e2b6)


## Installation & Setup

### One-Time Setup

Clone the repository along with its submodules:

```bash
git clone --recurse-submodules https://github.com/nimit/pg_gen_query.git
```

Copy `.env.template` to `.env` and fill in the required values.

Run the configuration script to add an environment variable to the PostgreSQL config (needed on Ubuntu where the Postgres worker is run separately):

```bash
bash scripts/configure_pg_gen_query.sh
```

A convenience script for building and installing the extension is provided at the repository root:

```bash
./build.sh
```

Finally, create the extension inside your database:

```sql
CREATE EXTENSION pg_gen_query;
```

## Usage

`pg_gen_query` accepts a natural language query and returns the SQL command that would produce the requested result. Internally, it uses ClickHouse's AI SDK along with a cached version of the database schema.

### Example

```sql
SELECT * FROM pg_gen_query("show me all products where price is greater than 20");
```

### Notes

- The extension currently supports only a single query at a time.
- It returns **only the generated SQL command**, not the actual data. PostgreSQL restrictions require queries returning `SETOF RECORD` to explicitly specify column keys, which prevents seamless data-returning behavior.

## Tests

Tests are organized into folders within the `tests` directory. Each folder contains a standalone `run.sh` script.

### Test Suites

- **01_concurrency**
  Runs multiple queries in parallel to benchmark schema cache performance on a populated database.
  **⚠️ Warning:** This test may consume a large number of AI credits due to many backend calls. To measure extension performance alone, disable AI SDK calls before running.

- **02_simple**
  Executes simple queries against a small database.

- **03_complex**
  Runs more complex queries on a larger and more intricate database.

- **04_reload_schema_on_change**
  Ensures the extension correctly invalidates and regenerates the schema cache when the underlying database schema changes.

## Roadmap

1. Add support for users to switch to using the more detailed schema as context.
2. Return actual query results instead of SQL strings. Because PostgreSQL requires `SETOF RECORD`, this would require the user to write: `SELECT * FROM pg_gen_query(query) AS (col1, col2);`.
3. Add support for processing multiple queries at once. Since most time is spent on network calls, batching could significantly improve performance.
4. Reduce schema size. Although human-readable now, the schema could be compacted using abbreviations and LLM-friendly encodings.
5. Investigate using PostgreSQL Dynamic Shared Memory to improve schema cache performance. Currently, schema data is cached per active connection; DSM may allow faster, lock-free reads.

> Note: AI tools were used in generating code/documentation for this extension.
