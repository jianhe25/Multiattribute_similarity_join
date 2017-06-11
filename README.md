### How to run
- ./cmake .
- ./sh run.sh

### Command line
- ./similarity_table_join ./dataset/mapping_rule /dataset/dblp_1k.table /dataset/dblp_1k.table
- ./similarity_table_search ./dataset/dblp_threshold_lowerbound ./dataset/dblp_1k.table ./dataset/dblp_query_1k
- --verify_exp_version: 2 is hybrid verification
- --max_base_table_size: max 1st table size
- --max_query_table_size: max 2nd table size
- --index_version: 3 is prefix tree search index, 4 is prefix tree join index.
- --baseline_exp: na is direct verification.

### Dataset
This project use DBLP dataset, include 1 base table, 1 query table, 1 mapping file, 1 threshold_lowerbound mapping file.
- Data table(./dataset/dblp_1k.table): There are 7 columns in DBLP dataset seperate by |
- Mapping file(./dataset/mapping_rule): has several rules, example of one rule, ES 3 3 0.8 means edit_distance between 3rd attribute of 1st table and 2nd table should larger than 0.8
- Threshold_lowerbound mapping file(./dataset/dblp_threshold_lowerbound): same as mapping file.
- Query table(./dataset/dblp_query_1k): each query is a record with a several rules.


