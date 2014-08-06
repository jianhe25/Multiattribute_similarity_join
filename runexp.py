from subprocess import call

for exp in range(6,9):
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

