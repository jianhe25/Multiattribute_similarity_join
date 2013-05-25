
l = open("dblp.table")
counter = 0
import random
out = open("ref.table", "w")
for i in range(100):
	for skip in range(random.randint(0, 100)):
		l.readline()
	line = l.readline()
	out.write(line)

