cmake_minimum_required(VERSION 3.5)
project(Multiattribute_similarity_join)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -ggdb -Wall")

set(SOURCE_FILES
    src/exp/Search0_NoEstimate.cc
    src/exp/Search1_Estimate.cc
    src/exp/Search2_TuneEstimate.cc
    src/lib/debugutils.cpp
    src/lib/debugutils.h
    src/lib/utils.cpp
    src/lib/utils.h
    src/prefix_index/prefix_index.cc
    src/prefix_index/prefix_index.h
    src/test/help_text.cc
    src/test/help_text.h
    src/tree_index/desicion_tree_index.cc
    src/tree_index/tree_index.h
    src/common.cc
    src/common.h
    src/core.cc
    src/core.h
    src/filter.cc
    src/filter.h
    src/sim_table.cc
    src/sim_table.h)

#add_executable(Multiattribute_similarity_join ${SOURCE_FILES})

add_definitions(-DDEBUG)
add_subdirectory(lib/gflags)
add_executable(similarity_table_join ${SOURCE_FILES} src/test/sim_table_join.cc)
add_executable(similarity_table_search ${SOURCE_FILES} src/test/sim_table_search.cc)

target_link_libraries(similarity_table_join gflags)
target_link_libraries(similarity_table_search gflags)