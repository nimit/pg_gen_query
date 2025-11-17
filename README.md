# pg_gen_query

This Postgres extension provides a function `pg_gen_query` that takes a natural language query and returns the SQL command that would generate the same result.

## Using the extension

#### One-time Setup

Clone the repository along with the submodules.

```bash
git clone --recurse-submodules https://github.com/nimit/pg_gen_query.git
```

Copy the `.env.template` file to `.env` and fill in the appropriate values.
Call the `configure_pg_gen_query.sh` script add an environent variable to the PostgreSQL config (this is required for the extension to work on Ubuntu as the Postgres worker is run ).

```bash
bash scripts/configure_pg_gen_query.sh
```

A convenience script to build and install the extension is provided at the root of the repositiory.

```bash
./build.sh
```

Run this command to create an extension on the database.

```sql
CREATE EXTENSION pg_gen_query;
```

#### Usage

The extension provides a function `pg_gen_query` that takes a natural language query and returns the SQL command. Under the hood, it will call Clickhouse's AI SDK with the query and a cached version of the schema to generate a SQL command.

Example:

```sql
SELECT * FROM pg_gen_query("show me all products where price is greater than 20");
```

The extension is currently limited to a single query at a time.
Note:
The extension currently only returns a SQL command instead of actual data because due to Postgres' restrictions, when returning type `SETOF RECORD`, the user needs to specify column keys.

## Tests

There are a few tests segregated in folders in the `tests` directory.
Each folder contains a `run.sh` script that is by itself, sufficient to run the tests.

01_concurrency: This test is a simple concurrency test that runs multiple queries in parallel (mainly used to test and benchmark the schema cache's performance on a populated database).
**DANGER: Running this test may use an inordinate amount of credits because it makes many calls to the extension (and therefore the AI backend).** To just measure the performace of the extension, disable calls to the AI SDK before running the test.

02_simple: Runs a few simple queries against a small database.

03_complex: Runs a few complex queries against a larger, more complex database.

04_reload_schema_on_change: Tests the extension's ability to automatically invalidate and regenerate the schema cache when the database schema changes.

## Roadmap

1. Return actual records instead of the SQL command (Postgres required the queries to be modeled as `SETOF RECORD` to work with the extension which would then require the function to be called as `SELECT * FROM pg_gen_query(query) AS col1, col2;`).
2. Add support for multiple queries. Since the processing time is dominated by the network call, batching multiple queries would be beneficial.
3. The schema is human-readable right now. But, since it is used as context to an LLM, its size can be reduced by using abbreviations and other techniques that the LLM can recognize.
4. Test if schema cache can be improved using Postgres' Dynamic Shared Memory & lockless reads. Currently, the schema is read and cached for each active connection inside the extension. I suspect using PG's DSM might be more effective.
