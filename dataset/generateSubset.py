import random
import sys

file1 = open(sys.argv[1])
for line in file1:
    if random.randint(0, 10) < 7:
        print line,

