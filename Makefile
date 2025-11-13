PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

EXTENSION = pg_gen_query
MODULE_big = pg_gen_query
OBJS = pg_gen_query.o

DATA = pg_gen_query--1.0.sql

CXX = g++
CXXFLAGS = -std=c++17 -fPIC -O2
PG_CPPFLAGS = -I$(shell $(PG_CONFIG) --includedir-server)

AI_SDK_DIR = $(CURDIR)/ai-sdk-cpp
AI_SDK_BUILD_DIR = $(AI_SDK_DIR)/build
AI_SDK_INCLUDE = $(AI_SDK_DIR)/include

CXXFLAGS += -I$(AI_SDK_INCLUDE)

SHLIB_LINK = \
  $(AI_SDK_BUILD_DIR)/libai-sdk-cpp-anthropic.a \
  $(AI_SDK_BUILD_DIR)/libai-sdk-cpp-core.a \
  $(AI_SDK_BUILD_DIR)/libai-sdk-cpp-openai.a \
  -lstdc++

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PG_CPPFLAGS) -c -o $@ $<

include $(PGXS)
