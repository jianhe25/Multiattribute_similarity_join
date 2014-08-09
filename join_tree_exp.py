from sys import argv
from subprocess import call

exp_version = int(argv[1])

if exp_version == 1:
    for exp in range(1,9):
        tau = 1 - exp * 0.1;
        ed = exp
        f = open('dataset/mapping_rule', 'w')
        f.write('JACCARD 1 1 ' + str(tau) + '\n')
        f.write('COSINE 2 2 ' + str(tau) + '\n')
        f.write('ED 3 3 ' + str(ed) + '\n')
        f.write('ED 7 7 ' + str(ed) + '\n')
        f.close()
        call(["make", "INDEX_VERSION=1"])
        call(["make", "INDEX_VERSION=3"])
        call(["make", "INDEX_VERSION=4"])

if exp_version == 2:
    for exp in range(1,9):
        tau = 0.95 - exp * 0.05;
        ed = exp
        f = open('dataset/mapping_rule', 'w')
        f.write('COSINE 2 2 ' + str(tau) + '\n')
        f.write('JACCARD 1 1 ' + str(tau) + '\n')
        f.write('ED 7 7 ' + str(ed) + '\n')
        f.write('ED 3 3 ' + str(ed) + '\n')
        f.close()
        call(["make", "INDEX_VERSION=2"])

if exp_version == 3:
    for exp in range(1, 9):
        tau = 1.0 - exp * 0.1;
        ed = exp
        f = open('dataset/mapping_rule', 'w')
        f.write('JACCARD 0 0 ' + str(tau) + '\n')
        f.write('COSINE 1 1 ' + str(tau) + '\n')
        f.write('ED 2 2 ' + str(ed) + '\n')
        f.write('ED 3 3 ' + str(ed) + '\n')
        f.close()
        call(["make", "INDEX_VERSION=1"])
        call(["make", "INDEX_VERSION=3"])
        call(["make", "INDEX_VERSION=4"])
