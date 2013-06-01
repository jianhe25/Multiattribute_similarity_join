CXX=g++-4.7
CXXFLAGS=-O3 -ggdb -Wall -std=c++11 -lgflags 
# Flags passed to the preprocessor.
CPPFLAGS = -I$(GTEST_DIR)/include

SRC_OBJ=./src/core.o ./src/sim_table.o ./src/common.o ./src/filter.o
EXP_OBJ=./src/exp/Search0_NoEstimate.o ./src/exp/Search1_Estimate.o ./src/exp/Search2_TuneEstimate.o
COMMON_OBJ=$(SRC_OBJ) $(EXP_OBJ)
TEST_OBJS=./test/filter_test.o
RUN_OBJ=./test/test_sim_table.o

RUN_ELF=./test_sim_table 

TESTS=./test/filter_test

tests : $(TESTS)
	./test/filter_test

./test/filter_test: $(COMMON_OBJ) ./test/filter_test.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

run: $(RUN_ELF)
	$(RUN_ELF) ./dataset/mapping_rule ./dataset/dblp.table ./dataset/ref.table --exp_version=1 --max_base_table_size=10000 --max_query_table_size=100
gdb: $(RUN_ELF)
	gdb $(RUN_ELF)

$(RUN_ELF) : $(COMMON_OBJ) $(RUN_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ 

clean:
	rm $(COMMON_OBJ) $(TEST_OBJS) $(RUN_OBJ) $(RUN_ELF)

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

