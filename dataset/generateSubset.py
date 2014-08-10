import random
import sys

file1 = open(sys.argv[1])
num = int(sys.argv[2])
lines = []
for line in file1:
    if random.randint(0, 1000000) > num:
        print line,
    if random.randint(0, 1000000) < num:
        lines.append(line);

for line in lines:
    print line,

