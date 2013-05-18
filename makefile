CXX=g++-4.7
CXXFLAGS=-O3 -ggdb -Wall -std=c++11 

SRC_OBJ=./src/core.o ./src/sim_table.o ./src/common.o ./src/filter.o
TEST_OBJ=./test/test_sim_table.o
OBJ=$(SRC_OBJ) $(TEST_OBJ)
TEST_ELF=./test_sim_table

test: $(TEST_ELF)
	$(TEST_ELF) ./dataset/mapping_rule

gdb: $(TEST_ELF)
	gdb $(TEST_ELF)

$(TEST_ELF) : $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TEST_ELF) 

clean:
	rm $(OBJ)

submit:
	make clean
	tar czf sim_table dataset makefile README src test

