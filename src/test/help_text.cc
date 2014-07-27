#include "help_text.h"



string mappingFileFormat=
"mapping_file format : 	 				    							\n\
=========================================================				\n\
ED 0 0 1 																\n\
JACCARD 1 1 0.9 														\n\
========================================================				\n\
Explaination:															\n\
	ED(0,0) = 1 means edit_distance between word in column 0 in table1 to word in column 0 in table2 <= 1 \n\
	JACCARD (1,1) = 0.9 means JACCARD_distance between string in column1 in table1 to string in column1 in table2 <= 1 \n\
";

string tableFileFormat =
"table_file format:														\n\
=========================================================				\n\
record00 | record01 | record02 | record03 | ....						\n\
record10 | record11 | record12 | record13 | ....						\n\
...																		\n\
=========================================================				\n\
";


string searchFileFormat =
"search_query_file format : 	 			    						\n\
=========================================================				\n\
#query 2																\n\
record00 | record01 | record02 | record03 | ....						\n\
ED 0 0 1 																\n\
JACCARD 1 1 0.9 														\n\
#query 2																\n\
record10 | record11 | record12 | record13 | ....						\n\
ED 1 1 3 																\n\
COS 2 2 0.5 														    \n\
#query 2																\n\
record20 | record21 | record22 | record23 | ....						\n\
ED 0 0 1 																\n\
DICE 1 1 0.9 															\n\
...																		\n\
========================================================				\n\
Explaination:															\n\
	ED(0,0) = 1 means edit_distance between word in column 0 in table1 to word in column 0 in table2 <= 1 \n\
	JACCARD (1,1) = 0.9 means JACCARD_distance between string in column1 in table1 to string in column1 in table2 <= 1 \n\
";

string joinCommand =
".\\sim_table_join mapping_file table_file1 table_file2 [--exp_version=] [--index_version=] [--max_base_table_size=] [--max_query_table_size=]\n";
string joinUsageText = "\n" + joinCommand + "\n" + mappingFileFormat + "\n" + tableFileFormat;

string searchCommand =
".\\sim_table_search table_file search_file [--exp_version=] [--index_version=] [--max_base_table_size=] [--max_query_table_size=]\n";
string searchUsageText = "\n" + searchCommand + "\n" + searchFileFormat + "\n" + tableFileFormat;


