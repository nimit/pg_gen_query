The extension returns SQL command instead of actual data because due to Postgres' restrictions, when returning type `SETOF RECORD`, the user needs to specify column keys.
