import sys
import datetime
from subprocess import call

max_base_table_size = 10000
for threshold in range(1, 10):
	print threshold
	mapping_config = open('dataset/mapping_config','w');
	mapping_config.write('ED 3 3 ' + str(threshold) + '\n');
	mapping_config.write('ED 5 5 ' + str(threshold) + '\n');
	mapping_config.write('JACCARD 2 2 ' + str(1.0 - float(threshold) / 10.0) + '\n');
	mapping_config.write('JACCARD 1 1 ' + str(1.0 - float(threshold) / 10.0) + '\n');
	mapping_config.close()

	#stat_file = open("stat_file.csv","w");
	#stat_file.write("");
	#stat_file.close();

	for version in range(0, 3):
#	for max_query_table_size in range(100, 100 * 10, 100):
		startTime = datetime.datetime.now()
		call(['./test_sim_table', 
			  'dataset/mapping_config', 
			  'dataset/dblp.table', 
			  'dataset/ref.table',
			  '--exp_version='+str(version),
			  '--max_base_table_size='+str(max_base_table_size),
			  '--index_version='+str(1)
			   ])
		endTime = datetime.datetime.now();
		print (endTime - startTime).microseconds,
	print ""

