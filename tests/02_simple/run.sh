#!/bin/bash

ORIG_DIR="$(pwd)"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DB=simple_test

echo "=== Initializing database ==="
psql -v ON_ERROR_STOP=1 -f init_state.sql postgres

echo "=== Running test queries ==="

declare -A TESTS

TESTS["List all customers"]="SELECT * FROM customers;"
TESTS["Show customers older than 30"]="SELECT * FROM customers WHERE age > 30;"
TESTS["Find all orders for Alice"]="SELECT o.* FROM orders o JOIN customers c ON o.customer_id = c.id WHERE c.name = 'Alice';"
TESTS["What items did Bob buy?"]="SELECT o.item FROM orders o JOIN customers c ON o.customer_id = c.id WHERE c.name = 'Bob';"

ALL_PASSED=1

for NL in "${!TESTS[@]}"; do
  EXPECTED_SQL="${TESTS[$NL]}"

  echo ""
  echo "-------------------------------------------"
  echo "Natural language: $NL"
  echo "-------------------------------------------"

  GENERATED_SQL=$(psql -d $DB -t -A -c "SELECT pg_gen_query('$NL');")

  echo "Generated SQL: $GENERATED_SQL"
  echo "Expected SQL : $EXPECTED_SQL"

  EXPECTED_OUT=$(psql -d $DB -t -A -c "$EXPECTED_SQL" | sort)
  GENERATED_OUT=$(psql -d $DB -t -A -c "$GENERATED_SQL" | sort)

  if [[ "$EXPECTED_OUT" == "$GENERATED_OUT" ]]; then
    echo "[PASS]"
  else
    echo "[FAIL]"
    echo "--- Expected Output ---"
    echo "$EXPECTED_OUT"
    echo "--- Generated Output ---"
    echo "$GENERATED_OUT"
    ALL_PASSED=0
  fi
done

echo ""
if [[ $ALL_PASSED -eq 1 ]]; then
  echo "=== ALL TESTS PASSED ==="
else
  echo "=== SOME TESTS FAILED ==="
fi

cd "$ORIG_DIR"