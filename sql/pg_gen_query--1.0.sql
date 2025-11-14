CREATE FUNCTION pg_gen_query(query text)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'pg_gen_query'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION regen_schema_cache()
RETURNS void
AS 'pg_gen_query', 'regen_schema_cache'
LANGUAGE C;

-- PL/pgSQL wrapper for the event trigger
CREATE FUNCTION regen_schema_cache_trigger()
RETURNS event_trigger
LANGUAGE plpgsql
AS $$
BEGIN
    PERFORM regen_schema_cache();
END;
$$;

-- Event trigger: fires on ANY DDL change
CREATE EVENT TRIGGER pg_gen_query_schema_trigger
ON ddl_command_end
EXECUTE FUNCTION regen_schema_cache_trigger();