PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

EXTENSION = pg_gen_query
MODULES = pg_gen_query
DATA = pg_gen_query--1.0.sql
OBJS = pg_gen_query.o

CXX = g++
CXXFLAGS = -std=c++17 -fPIC -O2
PG_CPPFLAGS = -I$(shell $(PG_CONFIG) --includedir-server)

# explicit rule to compile .cpp -> .o with g++
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PG_CPPFLAGS) -c -o $@ $<

include $(PGXS)
