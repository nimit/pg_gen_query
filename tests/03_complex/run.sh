#!/bin/bash

ORIG_DIR="$(pwd)"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DB=complex_test

echo "=== Initializing database ==="
psql -v ON_ERROR_STOP=1 -f init_state.sql postgres

echo "=== Running NL → SQL test queries ==="

# -----------------------------------------
# Expected SQL answers for complex schema
# -----------------------------------------

declare -A TESTS

TESTS["Show all orders shipped from the East region with customer name and city"]=" \
SELECT o.order_id, u.name, a.city \
FROM orders o \
JOIN users u ON o.user_id = u.user_id \
JOIN addresses a ON u.user_id = a.user_id \
JOIN shipments s ON o.order_id = s.order_id \
JOIN warehouses w ON s.warehouse_id = w.warehouse_id \
WHERE w.region = 'East';"

TESTS["List products that are low in stock below 20 units"]=" \
SELECT p.product_id, p.name, i.quantity
FROM products p
JOIN inventory i ON p.product_id = i.product_id
WHERE i.quantity < 20;"

TESTS["Which customers ordered electronics but never ordered clothing"]=" \
SELECT DISTINCT u.user_id, u.name \
FROM users u \
JOIN orders o ON u.user_id = o.user_id \
JOIN order_items oi ON o.order_id = oi.order_id \
JOIN products p ON oi.product_id = p.product_id \
WHERE p.category = 'Electronics' \
AND u.user_id NOT IN ( \
  SELECT u2.user_id \
  FROM users u2 \
  JOIN orders o2 ON u2.user_id = o2.user_id \
  JOIN order_items oi2 ON o2.order_id = oi2.order_id \
  JOIN products p2 ON oi2.product_id = p2.product_id \
  WHERE p2.category = 'Clothing' \
);"

TESTS["What were the total quantities shipped per region last month"]=" \
SELECT w.region, SUM(oi.quantity) AS total_quantity \
FROM shipments s \
JOIN warehouses w ON s.warehouse_id = w.warehouse_id \
JOIN orders o ON s.order_id = o.order_id \
JOIN order_items oi ON o.order_id = oi.order_id \
WHERE s.shipped_date >= date_trunc('month', CURRENT_DATE - INTERVAL '1 month') \
  AND s.shipped_date < date_trunc('month', CURRENT_DATE) \
GROUP BY w.region;"

# -----------------------------------------
# Execute tests
# -----------------------------------------

ALL_PASSED=1

for NL in "${!TESTS[@]}"; do
  EXPECTED_SQL=$(echo "${TESTS[$NL]}" | sed 's/^[ \t]*//')

  echo ""
  echo "-------------------------------------------"
  echo "Natural language query: $NL"
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