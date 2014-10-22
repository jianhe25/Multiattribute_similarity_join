from sys import argv
from subprocess import call

exp_version = int(argv[1])

if exp_version == 1:
    for exp in range(5,6):
        tau = 1 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.write('ES 7 7 ' + str(tau) + '\n')
        f.write('JACCARD 2 2 ' + str(tau) + '\n')
        f.close()
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=1", "VERIFY_EXP_VERSION=0"])
        call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])
        with open('formated_stat_file', 'a') as formated_stat_file:
            formated_stat_file.write('\n')

# compare edjoin- IMDB
if exp_version == 2:
    for exp in range(5, 6):
        tau = 1.0 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 2 2 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('ES 0 0 ' + str(tau) + '\n')
        f.write('JACCARD 1 1 ' + str(tau) + '\n')
        f.close()
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
        call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])
        with open('formated_stat_file', 'a') as formated_stat_file:
            formated_stat_file.write('\n')

# compare attr no
if exp_version == 3:
        tau = 0.8
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.write('ES 7 7 ' + str(tau) + '\n')
        f.close()
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3", "VERIFY_EXP_VERSION=2"])
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=1", "VERIFY_EXP_VERSION=0"])
        call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])

# compare edjoin- IMDB - attrno
if exp_version == 4:
    tau = 0.8
    f = open('dataset/mapping_rule', 'w')
    f.write('ES 2 2 ' + str(tau) + '\n')
    f.write('ES 3 3 ' + str(tau) + '\n')
    f.write('ES 0 0 ' + str(tau) + '\n')
    #f.write('JACCARD 1 1 ' + str(tau) + '\n')
    f.close()
    #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
    #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=1", "VERIFY_EXP_VERSION=0"])
    call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

if exp_version == 5:
    for exp in range(4, 6):
        tau = 1.0 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 2 2 ' + str(tau) + '\n')
        f.write('ES 0 0 ' + str(tau) + '\n')
        f.write('JACCARD 1 1 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.close()
        call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=2"])
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])

# compare orders
if exp_version == 6:
    for exp in range(5,6):
        tau = 1 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('JACCARD 2 2 ' + str(tau) + '\n')
        f.write('ES 7 7 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.close()
        call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=2"])
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
        with open('formated_stat_file', 'a') as formated_stat_file:
            formated_stat_file.write('\n')

# compare optimal tree - DBLP
if exp_version == 7:
    for exp in range(4,6):
        tau = 1 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('JACCARD 2 2 ' + str(tau) + '\n')
        f.write('ES 7 7 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.close()
        call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=4"])
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
        with open('formated_stat_file', 'a') as formated_stat_file:
            formated_stat_file.write('\n')

# compare VERIFY_EXP_VERSION - IMDB
if exp_version == 8:
    for exp in range(4, 6):
        tau = 1.0 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 2 2 ' + str(tau) + '\n')
        f.write('ES 0 0 ' + str(tau) + '\n')
        f.write('JACCARD 1 1 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.close()
        call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3", "VERIFY_EXP_VERSION=1"])

# compare VERIFY_EXP_VERSION - DBLP
if exp_version == 9:
    for exp in range(1,6):
        tau = 1 - exp * 0.05;
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 7 7 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('JACCARD 2 2 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.close()
        call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3", "VERIFY_EXP_VERSION=1"])
        with open('formated_stat_file', 'a') as formated_stat_file:
            formated_stat_file.write('\n')

# compare scalability - DBLP
if exp_version == 10:
    for exp in range(5, 6):
        max_table_size = exp * 200000
        tau = 0.8;
        f = open('dataset/mapping_rule', 'w')
        f.write('ES 7 7 ' + str(tau) + '\n')
        f.write('ES 3 3 ' + str(tau) + '\n')
        f.write('JACCARD 2 2 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.close()
        call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "TABLE1_SIZE="+str(max_table_size), "TABLE2_SIZE="+str(max_table_size), "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])
        with open('formated_stat_file', 'a') as formated_stat_file:
            formated_stat_file.write('\n')

# compare scalability - IMDB
if exp_version == 11:
    for exp in range(1, 6):
        max_table_size = exp * 200000
        call(["make", "TABLE1=./dataset/dblp_204.table", "TABLE2=./dataset/query/dblp_10w.query",  "TABLE1_SIZE="+str(max_table_size), "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3", "VERIFY_EXP_VERSION=2"])

# compare scalability - IMDB
if exp_version == 12:
    for exp in range(1, 6):
        max_table_size = exp * 200000
        call(["make", "TABLE1=./dataset/dblp_204.table", "TABLE2=./dataset/query/dblp_10w.query",  "TABLE1_SIZE="+str(max_table_size), "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])

    #for exp in range(4, 5):
        #tau = 0.8
        #max_table_size = exp * 100000
        #f = open('dataset/mapping_rule', 'w')
        #f.write('ES 2 2 ' + str(tau) + '\n')
        #f.write('ES 0 0 ' + str(tau) + '\n')
        #f.write('JACCARD 1 1 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table",  "TABLE1_SIZE="+str(max_table_size),"TABLE2_SIZE="+str(max_table_size), "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])

#if exp_version == 3:
    #for exp in range(1, 6):
        #tau = 1.0 - exp * 0.05;
        #f = open('dataset/mapping_rule', 'w')
        #f.write('COSINE 0 0 ' + str(tau) + '\n')
        #f.write('JACCARD 1 1 ' + str(tau) + '\n')
        #f.write('ES 2 2 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "INDEX_VERSION=1"])
        #call(["make", "INDEX_VERSION=3"])
        #call(["make", "INDEX_VERSION=4"])

#if exp_version == 4:
    #for exp in range(1, 6):
        #tau = 1.00 - exp * 0.05;
        #f = open('dataset/mapping_rule', 'w')
        #f.write('COSINE 0 0 ' + str(tau) + '\n')
        #f.write('JACCARD 1 1 ' + str(tau) + '\n')
        #f.write('ES 2 2 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "INDEX_VERSION=2"])
        #f = open('dataset/mapping_rule', 'w')
        #f.write('JACCARD 1 1 ' + str(tau) + '\n')
        #f.write('COSINE 0 0 ' + str(tau) + '\n')
        #f.write('ES 2 2 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "INDEX_VERSION=2"])
        #f = open('dataset/mapping_rule', 'w')
        #f.write('ES 2 2 ' + str(tau) + '\n')
        #f.write('COSINE 0 0 ' + str(tau) + '\n')
        #f.write('JACCARD 1 1 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "INDEX_VERSION=2"])
        ##f = open('dataset/mapping_rule', 'w')
        ##f.write('ES 3 3 ' + str(tau) + '\n')
        ##f.write('COSINE 0 0 ' + str(tau) + '\n')
        ##f.write('JACCARD 1 1 ' + str(tau) + '\n')
        ##f.write('ES 2 2 ' + str(tau) + '\n')
        ##f.close()
        ##call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "INDEX_VERSION=2"])
        #with open('formated_stat_file', 'a') as formated_stat_file:
            #formated_stat_file.write('\n')

## compare verification DBLP
#if exp_version == 5:
    #for exp in range(1, 6):
        #tau = 1.0 - exp * 0.05;
        #f = open('dataset/mapping_rule', 'w')
        #f.write('COSINE 1 1 ' + str(tau) + '\n')
        #f.write('JACCARD 2 2 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.write('ES 7 7 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "VERIFY_EXP_VERSION=2"])
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "VERIFY_EXP_VERSION=1"])
        #with open('formated_stat_file', 'a') as formated_stat_file:
            #formated_stat_file.write('\n')

## compare verification IMDB
#if exp_version == 6:
    #for exp in range(1, 6):
        #tau = 1.0 - exp * 0.05;
        #f = open('dataset/mapping_rule', 'w')
        #f.write('COSINE 0 0 ' + str(tau) + '\n')
        #f.write('JACCARD 1 1 ' + str(tau) + '\n')
        #f.write('ES 2 2 ' + str(tau) + '\n')
        #f.write('ES 3 3 ' + str(tau) + '\n')
        #f.close()
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "VERIFY_EXP_VERSION=2"])
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "VERIFY_EXP_VERSION=1"])
        #with open('formated_stat_file', 'a') as formated_stat_file:
            #formated_stat_file.write('\n')

