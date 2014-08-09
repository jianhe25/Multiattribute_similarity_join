CXX=g++-4.7
DEFINES = -DDEBUG #-DINTERSECT_PREFIX_LIST
CXXFLAGS=-O3 -ggdb -Wall -std=c++11 -I$(GTEST_DIR)/include $(DEFINES) 
LDFLAGS=-L /lib64 -l pthread
# Flags passed to the preprocessor.


SRC_OBJ=./src/core.o ./src/sim_table.o ./src/common.o ./src/filter.o ./src/lib/debugutils.o ./src/lib/utils.o
EXP_OBJ=./src/exp/Search0_NoEstimate.o ./src/exp/Search1_Estimate.o ./src/exp/Search2_TuneEstimate.o 
INDEX_OBJ=./src/prefix_index/prefix_index.o ./src/tree_index/tree_index.o

COMMON_OBJ=$(SRC_OBJ) $(EXP_OBJ) $(INDEX_OBJ)
TEST_OBJS=./src/test/filter_test.o
JOIN_OBJ=./src/test/sim_table_join.o ./src/test/help_text.o
SEARCH_OBJ=./src/test/sim_table_search.o ./src/test/help_text.o

JOIN_ELF=./sim_table_join
SEARCH_ELF=./sim_table_search

TESTS=./src/test/filter_test

EXP=2
INDEX_VERSION=3
# 1: single index
# 2: unordered-join tree
# 3: ordered-join tree
# 4: optimal-join tree
MEMORY_LIMIT=100
JOIN_ARGS=./dataset/mapping_rule ./dataset/imdb/imdb_35w.table ./dataset/imdb/imdb_35w.table --exp_version=$(EXP) --max_base_table_size=1000000 --max_query_table_size=1000000 --index_version=$(INDEX_VERSION)
SEARCH_ARGS=./dataset/rule_threshold_lowerbound ./dataset/dblp_80w.table ./dataset/dblp.query --exp_version=$(EXP) --max_base_table_size=1000000 --max_query_table_size=1000000 --index_version=$(INDEX_VERSION) --memory_limit=$(MEMORY_LIMIT)

join: $(JOIN_ELF)
	$(JOIN_ELF) $(JOIN_ARGS)

search: $(SEARCH_ELF)
	$(SEARCH_ELF) $(SEARCH_ARGS)

gdbs: $(SEARCH_ELF)
	gdb --args $(SEARCH_ELF) $(SEARCH_ARGS)

help: $(JOIN_ELF)
	$(JOIN_ELF) --help

$(JOIN_ELF) : $(COMMON_OBJ) $(JOIN_OBJ)
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ -lgflags

$(SEARCH_ELF) : $(COMMON_OBJ) $(SEARCH_OBJ)
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ -lgflags

$(TESTS): $(COMMON_OBJ) ./src/test/filter_test.o gtest_main.a
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ -lgflags $(LDFLAGS)

tests : $(TESTS)
	./src/test/filter_test

gdb: $(JOIN_ELF)
	gdb --args $(JOIN_ELF) $(JOIN_ARGS)


clean:
	rm -f $(COMMON_OBJ) $(TEST_OBJS) $(JOIN_OBJ) $(JOIN_ELF) *.o *.a

submit:
	make clean
	tar czf sim_table dataset makefile README src test































# Following part is for gtest framework, DON'T CHANGE!!!!!!!!!!
# Points to the root of Google Test, relative to where this file is.
# Remember to tweak this if you move this file.
GTEST_DIR = ./gtest-1.6.0

# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = sample1_unittest sample2_unittest

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

