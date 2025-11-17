#!/bin/bash

ORIG_DIR="$(pwd)"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

psql -f init_state.sql postgres
pgbench -c 100 -j 8 -T 180 -f benchmark.sql benchmark > benchmark.log 2>&1
cd "$ORIG_DIR"