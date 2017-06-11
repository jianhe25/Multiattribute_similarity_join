#!/usr/bin/env bash

make


VERIFY_EXP_VERSION=0
JOIN_INDEX_VERSION=4
SEARCH_INDEX_VERSION=3
# 1: single index
# 2: unordered-join tree
# 3: ordered-join tree, greedy tree
# 4: optimal-join tree
MEMORY_CONTROL=2
TABLE1=./dataset/dblp_1k.table
TABLE2=./dataset/dblp_1k.table
QUERY_TABLE=./dataset/dblp_query_1k
SEARCH_EXP=dynamic_search
BASELINE_EXP=edjoin+ppjoin
TABLE1_SIZE=1000000
TABLE2_SIZE=1000000
# dynamic_search
# verify_directly
# intersect_only

# Run Join
./similarity_table_join ./dataset/mapping_rule $TABLE1 $TABLE2 --verify_exp_version=$VERIFY_EXP_VERSION --max_base_table_size=$TABLE1_SIZE --max_query_table_size=$TABLE2_SIZE --index_version=$JOIN_INDEX_VERSION --baseline_exp=$BASELINE_EXP

# Run Search
./similarity_table_search ./dataset/dblp_threshold_lowerbound $TABLE1 $QUERY_TABLE --verify_exp_version=$VERIFY_EXP_VERSION --max_base_table_size=$TABLE1_SIZE --max_query_table_size=$TABLE2_SIZE --index_version=$SEARCH_INDEX_VERSION --memory_control=$MEMORY_CONTROL --search_exp=$SEARCH_EXP --baseline_exp=na
