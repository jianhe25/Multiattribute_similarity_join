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

if exp_version == 1:
    for exp in range(4, 6):
        mem_control = 1 + exp * 0.5
        call(["make", "MEMORY_CONTROL="+str(mem_control)])

# compare edjoin- IMDB - attrno
if exp_version == 2:
    call(["make", "search", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/query/imdb_10w.query", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=1"])
    call(["make", "search", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/query/imdb_10w.query", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5"])
    #call(["make", "TABLE1=./dataset/imdb/imdb_38w.table", "TABLE2=./dataset/imdb/imdb_38w.table", "BASELINE_EXP=edjoin+ppjoin", "INDEX_VERSION=5", "VERIFY_EXP_VERSION=0"])
    with open('formated_stat_file', 'a') as formated_stat_file:
        formated_stat_file.write('\n')
