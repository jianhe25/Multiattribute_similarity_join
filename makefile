CXX=g++-4.7
DEFINES = -DDEBUG #-DINTERSECT_PREFIX_LIST
CXXFLAGS=-O3 -ggdb -Wall -std=c++11 -I$(GTEST_DIR)/include $(DEFINES)
# Flags passed to the preprocessor.


SRC_OBJ=./src/core.o ./src/sim_table.o ./src/common.o ./src/filter.o ./src/lib/debugutils.o ./src/lib/utils.o
EXP_OBJ=./src/exp/Search0_NoEstimate.o ./src/exp/Search1_Estimate.o ./src/exp/Search2_TuneEstimate.o 
INDEX_OBJ=./src/index/index.o ./src/index/prefix_index/prefix_index.o ./src/tree_index/tree_index.o

COMMON_OBJ=$(SRC_OBJ) $(EXP_OBJ) $(INDEX_OBJ)
TEST_OBJS=./src/test/filter_test.o
RUN_OBJ=./src/test/test_sim_table.o

RUN_ELF=./test_sim_table 

TESTS=./src/test/filter_test

EXP=2
INDEX_VERSION=2
RUN_ARGS=./dataset/mapping_rule ./dataset/dblp_new.table ./dataset/dblp_new.table --exp_version=$(EXP) --max_base_table_size=1000000 --max_query_table_size=5000 --index_version=$(INDEX_VERSION)

run: $(RUN_ELF)
	$(RUN_ELF) $(RUN_ARGS)

$(RUN_ELF) : $(COMMON_OBJ) $(RUN_OBJ)
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ -lgflags

$(TESTS): $(COMMON_OBJ) ./src/test/filter_test.o gtest_main.a
	$(CXX) $(CXXFLAGS) $(DEFINES) $^ -o $@ -lgflags

tests : $(TESTS)
	./src/test/filter_test

gdb: $(RUN_ELF)
	gdb --args $(RUN_ELF) $(RUN_ARGS)


clean:
	rm -f $(COMMON_OBJ) $(TEST_OBJS) $(RUN_OBJ) $(RUN_ELF) *.o *.a

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

