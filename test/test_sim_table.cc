#include <iostream>
#include <stdio.h>
#include <string>
#include "../src/common.h"
#include "../src/sim_table.h"
using namespace std;

const int MAX_LINE_LENGTH = 10000;

DIST_TYPE getType(const string &operand) {
	if (operand == "ED")
		return ED;
	else
		return JACCARD;
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

void loadTable(string table_file_name, Table &table) {
	FILE *input_file = fopen(table_file_name.c_str(), "r");
	if (input_file == NULL) {
		cerr << "open FILE " + table_file_name + " error" << endl;
		throw "open FILE " + table_file_name + " error";
	}
	while (fgets(line, MAX_LINE_LENGTH, input_file)) {
		vector<string> strs;
		splitString(line, '|', strs);
		Row row;
		for (auto &word : strs) {
			stripString(word);
			row.push_back(Field(word));
		}
		table.push_back(row);
	}
}
void loadMapping(string mapping_file_name,
				Table &table1,
				Table &table2,
				vector<Similarity> &mappingPairs) {
	FILE *mapping_file = fopen(mapping_file_name.c_str(), "r");
	if (mapping_file == NULL) {
		cerr << "open FILE " + mapping_file_name + " error" << endl;
		throw "open FILE " + mapping_file_name + " error";
	}
	int col1, col2;
	double dist;
	char operand[100];
	mappingPairs.clear();
	while (fscanf(mapping_file, "%s %d %d %lf", operand, &col1, &col2, &dist) != EOF) {
		mappingPairs.push_back(Similarity(col1, col2, dist, getType(operand)));
	}
}

void print(Row tuple) {
	for (auto field : tuple)
		cout << field.str << "\t|";
	cout << endl;
}
int main(int argc, char **argv) {
	if (argc > 4) {
		printf("Usage : ./testMultiJoin [mapping_file] [table_file1] [table_file2] > result");
		showJoinQueryFormat();
		// TODO : support this query format
		// showSearchQueryFormat();
	}
	string table1_file_name = "dataset/ref.table";
	string table2_file_name = "dataset/ref1.table";
	string mapping_file_name = "";
	vector<Similarity> mappingPairs;

	if (argc >= 2)
		mapping_file_name = argv[1];
	if (argc >= 3)
		table1_file_name = argv[2];
	if (argc >= 4)
		table2_file_name = argv[3];

	try {
		string f1 = table1_file_name;
		loadTable(table1_file_name, table1);
		loadTable(table2_file_name, table2);
		if (mapping_file_name != "")
			loadMapping(mapping_file_name, table1, table2, mappingPairs);
	} catch (char *errorMsg) {
		cerr << errorMsg << endl;
		return -1;
	}
	SimTable sim_table;
	vector<pair<RowID, RowID>> sim_pairs = sim_table.Join(table1, table2, mappingPairs);

	/*
	 *	Output sim_pairs
	 */
	for (auto pair : sim_pairs) {
		auto tuple1 = table1[pair.first];
		auto tuple2 = table2[pair.second];
		printf("simPair %d %d\n",pair.first, pair.second);
		print(tuple1);
		print(tuple2);
		puts("");
	}
}


