#!/bin/bash

ORIG_DIR="$(pwd)"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DB=change_schema_test

echo "=== Initializing database ==="
psql -v ON_ERROR_STOP=1 -f init_state.sql postgres

echo "=== Running BEFORE-change test queries ==="

declare -A TESTS_BEFORE

declare -A TESTS_BEFORE=(
  ["List all users"]="
    SELECT id, name, email, created_at
    FROM users
    ORDER BY id;
  "
  ["Show all products in electronics category"]="
    SELECT id, name, category, price
    FROM products
    WHERE category = 'Electronics'
    ORDER BY id;
  "
  # ["Get orders for user 1"]="
  #   SELECT id, user_id, total, created_at
  #   FROM orders
  #   WHERE user_id = 1
  #   ORDER BY id;
  # "
  # ["Get order items for order 1"]="
  #   SELECT id, order_id, product_id, quantity, unit_price
  #   FROM order_items
  #   WHERE order_id = 1
  #   ORDER BY id;
  # "
)


ALL_PASSED=1

#############################################
# Run BEFORE-change tests
#############################################
for NL in "${!TESTS_BEFORE[@]}"; do
  EXPECTED_SQL="${TESTS_BEFORE[$NL]}"

  echo ""
  echo "-------------------------------------------"
  echo "Natural language (before): $NL"
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

#############################################
# Apply schema changes
#############################################

echo ""
echo "=== Applying schema changes ==="
psql -v ON_ERROR_STOP=1 -f change_schema.sql postgres

#############################################
# AFTER-change tests
#############################################

echo ""
echo "=== Running AFTER-change test queries ==="

declare -A TESTS_AFTER=(
  ["List users with their loyalty points"]="
    SELECT id, name, email, loyalty_points
    FROM users
    ORDER BY id;
  "
  ["List all orders including their status"]="
    SELECT id, user_id, total, status, created_at
    FROM orders
    ORDER BY id;
  "
  ["Show all coupons"]="
    SELECT id, code, discount
    FROM coupons
    ORDER BY id;
  "
  ["List coupons applied to each order"]="
    SELECT oc.order_id,
           oc.coupon_id,
           c.code,
           c.discount
    FROM order_coupons oc
    JOIN coupons c ON oc.coupon_id = c.id
    ORDER BY oc.order_id;
  "
  ["Get orders for user 1 including status and coupons"]="
    SELECT o.id,
           o.user_id,
           o.total,
           o.status,
           c.code
    FROM orders o
    LEFT JOIN order_coupons oc ON o.id = oc.order_id
    LEFT JOIN coupons c ON oc.coupon_id = c.id
    WHERE o.user_id = 1
    ORDER BY o.id;
  "
)

#############################################
# Run AFTER-change tests
#############################################

for NL in "${!TESTS_AFTER[@]}"; do
  EXPECTED_SQL="${TESTS_AFTER[$NL]}"

  echo ""
  echo "-------------------------------------------"
  echo "Natural language (after): $NL"
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

#############################################
# Summary
#############################################

echo ""
if [[ $ALL_PASSED -eq 1 ]]; then
  echo "=== ALL TESTS PASSED ==="
else
  echo "=== SOME TESTS FAILED ==="
fi

cd "$ORIG_DIR"
