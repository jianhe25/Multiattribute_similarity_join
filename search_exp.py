from sys import argv
from subprocess import call

exp_version = int(argv[1])

def rewrite_query_file(tau):
    new_query_f = open("./dataset/query/dblp_10w.query", "w")
    with open("./dataset/query/dblp_10w_base.query") as f:
        for line in f:
            parts = line.split()
            if len(parts) == 4:
                parts[3] = str(tau)
                new_query_f.write(" ".join(parts) + "\n")
            else:
                new_query_f.write(line)

def generate_dblp_lowerbound(tau):
    f = open('dataset/threshold_lowerbound', 'w')
    f.write('JACCARD 1 ' + str(tau) + '\n')
    f.write('JACCARD 2 ' + str(tau) + '\n')
    f.write('ES 3 ' + str(tau) + '\n')
    f.write('ES 7 ' + str(tau) + '\n')
    f.close()

def generate_imdb_lowerbound(tau):
    f = open('dataset/threshold_lowerbound', 'w')
    f.write('COSINE 0 ' + str(tau) + '\n')
    f.write('JACCARD 1 ' + str(tau) + '\n')
    f.write('ES 2 ' + str(tau) + '\n')
    f.write('ES 3 ' + str(tau) + '\n')
    f.close()

# compare DBLP 
if exp_version == 1:
    generate_dblp_lowerbound(0.7);
    call(["make", "search", "TABLE1=./dataset/dblp_100w_join.table", "TABLE2=./dataset/query/dblp_10w.query", "BASELINE_EXP=na", "INDEX_VERSION=6", "TABLE1_SIZE=1000000"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

# compare IMDB 
if exp_version == 2:
    generate_imdb_lowerbound(0.7);
    call(["make", "search", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/query/imdb_10w.query", "BASELINE_EXP=na", "INDEX_VERSION=6", "TABLE1_SIZE=400000"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')

