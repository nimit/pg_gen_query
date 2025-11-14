#!/bin/bash

# cd ai-sdk-cpp
# bash build.sh
# cd ..

make clean
make
sudo make install

psql -d test
# DROP EXTENSION IF EXISTS pg_gen_query CASCADE;
# CREATE EXTENSION pg_gen_query;
# SELECT * FROM pg_gen_query("show me all products where price is greater than 20");