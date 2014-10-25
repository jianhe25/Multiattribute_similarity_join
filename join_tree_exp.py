from sys import argv
from subprocess import call

exp_version = int(argv[1])

def generate_dblp_mapping_file(tau):
    f = open('dataset/mapping_rule', 'w')
    f.write('ES 3 3 ' + str(tau) + '\n')
    f.write('COSINE 1 1 ' + str(tau) + '\n')
    f.write('ES 7 7 ' + str(tau) + '\n')
    f.write('JACCARD 2 2 ' + str(tau) + '\n')
    f.close()

def generate_imdb_mapping_file(tau):
    f = open('dataset/mapping_rule', 'w')
    f.write('ES 2 2 ' + str(tau) + '\n')
    f.write('ES 3 3 ' + str(tau) + '\n')
    f.write('ES 0 0 ' + str(tau) + '\n')
    f.write('JACCARD 1 1 ' + str(tau) + '\n')
    f.close()

# compare baseline - DBLP
if exp_version == 1:
    exp = 4
    tau = 1 - exp * 0.05;
    generate_dblp_mapping_file(tau);
    call(["make", "join", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=6", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

# compare baseline - IMDB
if exp_version == 2:
    exp = 5
    tau = 1.0 - exp * 0.05;
    generate_imdb_mapping_file(tau);
    call(["make", "join", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=6", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

# compare DBLP attrno
if exp_version == 3:
    tau = 0.8
    f = open('dataset/mapping_rule', 'w')
    f.write('ES 3 3 ' + str(tau) + '\n')
    f.write('COSINE 1 1 ' + str(tau) + '\n')
    #f.write('ES 7 7 ' + str(tau) + '\n')
    f.close()
    call(["make", "join", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=6", "VERIFY_EXP_VERSION=0"])

# compare IMDB attrno
if exp_version == 4:
    tau = 0.8
    f = open('dataset/mapping_rule', 'w')
    f.write('ES 2 2 ' + str(tau) + '\n')
    f.write('ES 3 3 ' + str(tau) + '\n')
    #f.write('ES 0 0 ' + str(tau) + '\n')
    f.close()
    call(["make", "join", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=6", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

# compare scalability - DBLP
if exp_version == 5:
    max_table_size = 5 * 200000
    tau = 0.8
    generate_dblp_mapping_file(tau)
    call(["make", "join", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "TABLE1_SIZE="+str(max_table_size), "TABLE2_SIZE="+str(max_table_size), "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=6", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

# compare scalability - IMDB
if exp_version == 6:
    exp = 1
    max_table_size = exp * 100000
    tau = 0.8
    generate_imdb_mapping_file(tau)
    call(["make", "join", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "TABLE1_SIZE="+str(max_table_size), "TABLE2_SIZE="+str(max_table_size), "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=6", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

## compare orders
#if exp_version == 6:
    #for exp in range(5,6):
        #tau = 1 - exp * 0.05;
        #generate_dblp_mapping_file(tau);
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=2"])
        ##call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
        #with open('formated_stat_file', 'a') as formated_stat_file:
            #formated_stat_file.write('\n')

## optimal tree vs greedy tree - DBLP
#if exp_version == 7:
    #for exp in range(4,6):
        #tau = 1 - exp * 0.05;
        #generate_dblp_mapping_file(tau);
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=4"])
        ##call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3"])
        #with open('formated_stat_file', 'a') as formated_stat_file:
            #formated_stat_file.write('\n')

## compare VERIFY_EXP_VERSION - IMDB
#if exp_version == 8:
    #for exp in range(4, 6):
        #tau = 1.0 - exp * 0.05;
        #generate_imdb_mapping_file(tau);
        #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3", "VERIFY_EXP_VERSION=1"])

## compare VERIFY_EXP_VERSION - DBLP
#if exp_version == 9:
    #for exp in range(1,6):
        #tau = 1 - exp * 0.05;
        #generate_dblp_mapping_file(tau);
        #call(["make", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/dblp_100w_join.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=3", "VERIFY_EXP_VERSION=1"])
        #with open('formated_stat_file', 'a') as formated_stat_file:
            #formated_stat_file.write('\n')



