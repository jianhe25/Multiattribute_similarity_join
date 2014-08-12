import random
import sys
data_file = open("./dblp_204.table")
rule_file = open("./rule_threshold_lowerbound")

rules = []
for line in rule_file:
    rule = line.split();
    rules.append(rule)

line_no = 0
prob = [0.8,  #Jaccard 1 1 
        0.5,  #Jaccard 2 2
        0.3,  #ES 3 3
        0.1]  #ES 7 7 

lines = []
for line in data_file:
    lines.append(line)
#random.shuffle(lines)

#num_query = int(sys.argv[1])
queryid = 1
for line in lines:
    if queryid < 100000:
        query_rule = []
        while len(query_rule) <= 1:
            query_rule = []
            for i in range(0, len(rules)):
                if random.random() < prob[i]:
                    query_rule.append(rules[i])
        assert len(query_rule) > 1 and len(query_rule) <= 4
        #if len(query_rule) == 1 and query_rule[0][1] == "7":
            #query_rule.append(rules[random.randint(0, 2)])

        print "#query " + str(queryid) + " " + str(len(query_rule))
        print line,
        queryid += 1
        random.shuffle(query_rule)
        for rule in query_rule:
            threshold = float(rule[2])
            new_threshold = 0.8
            print rule[0] + " " + rule[1] + " " + rule[1] + " " + str(new_threshold)
        print
