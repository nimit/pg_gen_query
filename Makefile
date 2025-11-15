PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

EXTENSION = pg_gen_query
MODULE_big = pg_gen_query
OBJS = pg_gen_query.o guc.o generate_sql.o regen_schema.o

DATA = sql/pg_gen_query--1.0.sql

AI_SDK_DIR = $(CURDIR)/ai-sdk-cpp
AI_SDK_BUILD_DIR = $(AI_SDK_DIR)/build
AI_SDK_INCLUDE = $(AI_SDK_DIR)/include

CXX = g++
CXXFLAGS = -std=c++17 -fPIC -O2
PG_CPPFLAGS = -I$(shell $(PG_CONFIG) --includedir-server)
PG_CPPFLAGS += -I$(AI_SDK_DIR)/third_party/nlohmann_json_patched/include
PG_CPPFLAGS += -I$(AI_SDK_INCLUDE)
PG_CPPFLAGS += -DAI_SDK_HAS_OPENAI=1 -DAI_SDK_HAS_ANTHROPIC=1

AI_SDK_LIBS = \
  $(AI_SDK_BUILD_DIR)/libai-sdk-cpp-anthropic.so \
  $(AI_SDK_BUILD_DIR)/libai-sdk-cpp-core.so \
  $(AI_SDK_BUILD_DIR)/libai-sdk-cpp-openai.so \
  $(AI_SDK_BUILD_DIR)/third_party/brotli-cmake/brotli/libbrotlicommon.so \
  $(AI_SDK_BUILD_DIR)/third_party/brotli-cmake/brotli/libbrotlienc.so \
  $(AI_SDK_BUILD_DIR)/third_party/brotli-cmake/brotli/libbrotlidec.so \
  $(AI_SDK_BUILD_DIR)/third_party/zlib-cmake/zlib/libz.so

SHLIB_LINK = $(AI_SDK_LIBS) -lstdc++

# dynamically linking files by adding the shared libs to postgresql lib (solving error: "libai-sdk-cpp-anthropic.so: cannot open shared object file: No such file or directory" when creating extension)
PG_LDFLAGS += -Wl,-rpath,'$$ORIGIN'
PG_PKGLIBDIR := $(shell $(PG_CONFIG) --pkglibdir)
copy_ai_sdk_libs:
	@echo "Copying AI SDK shared libraries into $(PG_PKGLIBDIR)..."
	sudo cp $(AI_SDK_LIBS) $(PG_PKGLIBDIR)/

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PG_CPPFLAGS) -c -o $@ $<

include $(PGXS)
