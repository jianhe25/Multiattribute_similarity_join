#include <gflags/gflags.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include "../common.h"
#include "../sim_table.h"
#include "../tree_index/tree_index.h"

using namespace std;

const int MAX_LINE_LENGTH = 10000;
const int MAX_COLUMN_NUM = 20;

DEFINE_int32(max_base_table_size, 1000, "max tuple number in table1");
DEFINE_int32(max_query_table_size, 100, "max tuple number in table2");

DIST_TYPE getType(const string &operand) {
	if (operand == "ED")
		return ED;
	if (operand == "JACCARD")
		return JACCARD;
	return NON_DEFINE;
}
void showSearchQueryFormat() {
	printf("search_query_file format : \n\
			SELECT * WHERE 		\n\
			ED(column_name, your query word) = edit_distance(integer) AND(OR) \
			JACCARD(column_name, your query word) = similarity(float number, range in [0, 1] \
			e.g. \
			SELECT * WHERE \
			ED(author, EF codd) = 1 AND \
			JACCARD(title, a relational view of database) = 0.9 AND \
			example can be viewed in dataset/sample.query\n");
}

void showJoinQueryFormat() {
printf("Mapping_file format : 	 										\n\
=========================================================				\n\
ED 0 0 1 																\n\
JACCARD 1 1 0.9 														\n\
========================================================				\n\
Explaination:															\n\
	ED(0,0) = 1 means edit_distance between word in column 0 in table1 to word in column 0 in table2 <= 1 \n\
	JACCARD (1,1) = 0.9 means JACCARD_distance between string in column1 in table1 to string in column1 in table2 <= 1 \n\
Table_file format:														\n\
=========================================================				\n\
record00 | record01 | record02 | record03 | ....						\n\
record10 | record11 | record12 | record13 | ....						\n\
...																		\n\
=========================================================				\n\
");
}
int columnNum;
Table table1;
Table table2;
char line[MAX_LINE_LENGTH];

void loadTable(string table_file_name, Table &table, int read_limit) {
	FILE *input_file = fopen(table_file_name.c_str(), "r");
	if (input_file == NULL) {
		cerr << "open FILE " + table_file_name + " error" << endl;
		throw "open FILE " + table_file_name + " error";
	}
	int column_number = -1;
	int rowid = 0;
	while (fgets(line, MAX_LINE_LENGTH, input_file)) {
		vector<string> strs;
		splitString(line, '|', strs);
		// Validate column_number
		if (column_number != -1 && (int)strs.size() != column_number) {
			//cerr << "Warning : table row has different column number" << " tuple_number = " << table.size() << endl;
			continue;
		}
		column_number = strs.size();
		Row row;
		for (auto &word : strs) {
			stripString(word);
			row.push_back(Field(word, rowid));
		}
		rowid++;
		table.push_back(row);
		if ((int)table.size() >= read_limit)
			break;
	}
	fclose(input_file);
}
void loadMapping(string mapping_file_name,
				Table &table1,
				Table &table2,
				vector<Similarity> &mapping_pairs) {
	FILE *mapping_file = fopen(mapping_file_name.c_str(), "r");
	if (mapping_file == NULL) {
		cerr << "open FILE " + mapping_file_name + " error" << endl;
		throw "open FILE " + mapping_file_name + " error";
	}
	int col1, col2;
	double dist;
	char operand[100];
	mapping_pairs.clear();
	while (fscanf(mapping_file, "%s %d %d %lf", operand, &col1, &col2, &dist) != EOF) {
		if (getType(operand) == NON_DEFINE) {
			cerr << "NonExist Similarity Function" << endl;
			break;
		}
		mapping_pairs.push_back(Similarity(col1, col2, dist, getType(operand)));
	}
	puts("Mapping rules are:");
	for (auto sim : mapping_pairs) {
		cout << sim.colx << " " << sim.coly << " " << sim.dist << endl;
	}
	puts("==================================");
	fclose(mapping_file);
}

void print(Row tuple) {
	for (auto field : tuple)
		cout << field.str << "\t|";
	cout << endl;
}
void GenerateTokensOrGram(const vector<Similarity> &mapping_pairs) {
	// GenerateTokens or GenerateGrams
	for (const auto &sim : mapping_pairs) {
		for (unsigned i = 0; i < table1.size(); ++i) {
			if (sim.distType == JACCARD)
				table1[i][ sim.colx ].GenerateTokens();
			else
				table1[i][ sim.colx ].GenerateGrams();
		}
		for (unsigned i = 0; i < table2.size(); ++i) {
			if (sim.distType == JACCARD)
				table2[i][ sim.coly ].GenerateTokens();
			else
				table2[i][ sim.coly ].GenerateGrams();
		}
	}
}
int main(int argc, char **argv) {
	google::ParseCommandLineFlags(&argc, &argv, true);
    cout << FLAGS_exp_version << endl;
	if (argc > 4) {
		printf("Usage : ./testTableJoin [mapping_file] [table_file1] [table_file2] > result");
		showJoinQueryFormat();
		// TODO : support search query format
		// showSearchQueryFormat();
	}
	string table1_file_name = "dataset/dblp.table";
	string table2_file_name = "dataset/ref.table";
	string mapping_file_name = "dataset/mapping_rule";
	vector<Similarity> mapping_pairs;

	if (argc >= 2)
		mapping_file_name = argv[1];
	if (argc >= 3)
		table1_file_name = argv[2];
	if (argc >= 4)
		table2_file_name = argv[3];

	double time = getTimeStamp();
	try {
		loadTable(table1_file_name, table1, FLAGS_max_base_table_size);
		loadTable(table2_file_name, table2, FLAGS_max_query_table_size);
		if (mapping_file_name != "")
			loadMapping(mapping_file_name, table1, table2, mapping_pairs);
	} catch (char *errorMsg) {
		cerr << errorMsg << endl;
		return -1;
	}
	printf("Load table time: %.3fs\n", getTimeStamp() - time);
	time = getTimeStamp();
	GenerateTokensOrGram(mapping_pairs);
	printf("GenerateTokensOrGram time: %.3fs\n", getTimeStamp() - time);
	time = getTimeStamp();

    vector<pair<RowID, RowID>> sim_pairs;
	if (FLAGS_exp_version == 4) {
        TreeIndex tree_index(0);
        sim_pairs = tree_index.Join(table1, table2, mapping_pairs);
    } else {
        SimTable sim_table;
        sim_pairs = sim_table.Join(table1, table2, mapping_pairs);
    }

	/*
	 *	Output sim_pairs
	 */

	printf("sim_pairs.size() = %d\n", sim_pairs.size());
	//freopen("result","w",stdout);
	//for (auto pair : sim_pairs) {
		//auto tuple1 = table1[pair.first];
		//auto tuple2 = table2[pair.second];
		//if (tuple1[0].str != tuple2[0].str) {
			//print(tuple1);
			//print(tuple2);
			//puts("");
		//}
	//}
	//freopen("/dev/stdout", "w", stdout);

	double delta = getTimeStamp() - time;
	//printf("Your program has successfully passed all tests.\n");
	printf("Join time: %.2fs\n", getTimeStamp() - time);
	try {
		FILE *stat_file = fopen("stat_file.csv","ap");
        fprintf(stat_file, "exp_version %d : (%.2f, %d)\n", FLAGS_exp_version, delta, (int)sim_pairs.size());
	} catch (char *errorMsg) {
		cerr << errorMsg << endl;
		return -1;
	}
}


