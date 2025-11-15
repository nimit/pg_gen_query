#!/usr/bin/env bash
set -euo pipefail

echo "=== pg_gen_query configuration helper ==="

########################################
# Step 1: Detect available clusters
########################################

echo "[+] Detecting PostgreSQL clusters..."

if ! command -v pg_lsclusters >/dev/null 2>&1; then
  echo "ERROR: pg_lsclusters not found. Install 'postgresql-common' package."
  exit 1
fi

clusters=$(pg_lsclusters --no-header | awk '{print $1 ":" $2}')

if [ -z "$clusters" ]; then
  echo "ERROR: No PostgreSQL clusters found."
  exit 1
fi

if [ "$(echo "$clusters" | wc -l)" -gt 1 ]; then
  echo "Multiple clusters detected:"
  echo "$clusters" | nl
  read -rp "Select cluster number: " choice
  selected=$(echo "$clusters" | sed -n "${choice}p")
else
  selected="$clusters"
fi

pgver="${selected%%:*}"
cluster="${selected##*:}"

echo "[+] Selected cluster: version=$pgver, name=$cluster"

########################################
# Step 2: Locate postgresql.conf
########################################

conf="/etc/postgresql/$pgver/$cluster/postgresql.conf"

if [ ! -f "$conf" ]; then
  echo "ERROR: Could not locate postgresql.conf"
  exit 1
fi

echo "[+] Using config file: $conf"

########################################
# Step 3: Create backup
########################################

timestamp=$(date +"%Y-%m-%dT%H-%M-%S")
# backup="${conf}.backup-${timestamp}"
backup="${conf}.backup"

echo "[+] Creating backup: $backup"
sudo cp "$conf" "$backup"
sudo chmod 640 "$backup"
sudo chown postgres:postgres "$backup"

########################################
# Step 4: Load key from .env
########################################

if [ ! -f ".env" ]; then
  echo "ERROR: .env file not found in current directory."
  exit 1
fi

echo "[+] Loading .env file..."
set -a
source .env
set +a

key=""
guc_name=""

if [ ! -z "${OPENAI_API_KEY:-}" ]; then
  key="$OPENAI_API_KEY"
  guc_name="ai.openai_api_key"
elif [ ! -z "${ANTHROPIC_API_KEY:-}" ]; then
  key="$ANTHROPIC_API_KEY"
  guc_name="ai.anthropic_api_key"
else
  echo "ERROR: Neither OPENAI_API_KEY nor ANTHROPIC_API_KEY found in .env"
  exit 1
fi

echo "[+] Using GUC: $guc_name"

########################################
# Step 5: Apply modification safely
########################################

tmp="/tmp/pg_gen_query.conf.tmp"

echo "[+] Updating configuration..."

# Remove prior settings for our GUC
grep -v "^$guc_name" "$conf" > "$tmp"

# Append our updated GUC
{
  echo ""
  echo "# Added automatically by pg_gen_query config helper on $timestamp"
  echo "$guc_name = '${key}'"
} >> "$tmp"

# Replace original file atomically
sudo mv "$tmp" "$conf"
sudo chmod 644 "$conf"
sudo chown postgres:postgres "$conf"

echo "[+] postgresql.conf updated."
echo "[i] Backup saved at:"
echo "    $backup"

########################################
# Step 6: Reload PostgreSQL
########################################

echo "[+] Reloading PostgreSQL cluster..."
sudo systemctl reload "postgresql@$pgver-$cluster"

echo "=== Done! ==="
