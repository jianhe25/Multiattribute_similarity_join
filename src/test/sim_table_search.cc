#include <gflags/gflags.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include "../common.h"
#include "../sim_table.h"
#include "../tree_index/tree_index.h"
#include "help_text.h"

using namespace std;

const int MAX_LINE_LENGTH = 10000;

DEFINE_int32(max_base_table_size, 1000, "max tuple number in table1");
DEFINE_int32(max_query_table_size, 1000, "max tuple number in table2");

DIST_TYPE getType(const string &operand) {
	if (operand == "ED")
		return ED;
	if (operand == "JACCARD")
		return JACCARD;
	return NON_DEFINE;
}

int columnNum;
Table table;
vector<Query> queries;

void PrintSims(const vector<Similarity> &sims) {
	puts("==================================");
	puts("Mapping rules:");
	for (auto sim : sims) {
		string type;
		if (sim.distType == ED) {
			cout << "ED" << " (" << sim.colx << ", " << sim.coly << ") < " << sim.dist << endl;
		} else {
			cout << "JACCARD" << " (" << sim.colx << ", " << sim.coly << ") > " << sim.dist << endl;
		}
	}
	puts("==================================");
}

void loadDataTable(string table_file_name, Table *table, int read_limit) {
	char line[MAX_LINE_LENGTH];
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
		table->push_back(row);
		if ((int)table->size() >= read_limit)
			break;
	}
	fclose(input_file);
}

void loadQueries(string query_file_name, vector<Query> *queries, int read_limit) {
	/* read file */
	char line[MAX_LINE_LENGTH];
	FILE *query_file = fopen(query_file_name.c_str(), "r");
	if (query_file == NULL) {
		cerr << "open FILE " + query_file_name + " error" << endl;
		throw "open FILE " + query_file_name + " error";
	}
	/* parse data */
	int rowid = 0;
	int col1, col2;
	double dist;
	char operand[100];
	int num_sim;
	while (fgets(line, MAX_LINE_LENGTH, query_file))
	if (line[0] == '#') {
		Query query;
		vector<string> strs;
		sscanf(line, "#query %d", &num_sim);

		/* read query row */
		print_debug("num_sim = %d\n", num_sim);
		fgets(line, MAX_LINE_LENGTH, query_file);
		print_debug("line = %s\n",line);
		splitString(line, '|', strs);
		int num_col = strs.size();
		// Validate column_number
		if (num_col != -1 && (int)strs.size() != num_col) {
			cerr << "Warning : table row has different column number" << " tuple_number = " << table.size() << endl;
			continue;
		}
		for (auto &word : strs) {
			stripString(word);
			query.row.push_back(Field(word, rowid));
			query.id = rowid;
		}
		++rowid;

		/* read Similarity */
		for (int i = 0; i < num_sim; ++i) {
			fgets(line, MAX_LINE_LENGTH, query_file);
			sscanf(line, "%s %d %d %lf", operand, &col1, &col2, &dist);
			if (getType(operand) == NON_DEFINE) {
				print_debug("NonExist Similarity Function %s\n", operand);
				break;
			}
			query.sims.push_back( Similarity(col1, col2, dist, getType(operand)) );
		}
		PrintSims(query.sims);
		queries->push_back(query);
		if ((int)queries->size() >= read_limit)
			break;
	}
	fclose(query_file);
}

vector<Similarity> loadMapping(string mapping_file_name) {
	vector<Similarity> mapping_pairs;
	FILE *mapping_file = fopen(mapping_file_name.c_str(), "r");
	if (mapping_file == NULL) {
		cerr << "open FILE " + mapping_file_name + " error" << endl;
		throw "open FILE " + mapping_file_name + " error";
	}
	int col1;
	double dist;
	char operand[100];
	mapping_pairs.clear();
	while (fscanf(mapping_file, "%s %d %lf", operand, &col1, &dist) != EOF) {
		if (getType(operand) == NON_DEFINE) {
			print_debug("NonExist Similarity Function %s\n", operand);
			break;
		}
		mapping_pairs.push_back(Similarity(col1, -1, dist, getType(operand)));
	}
	PrintSims(mapping_pairs);
	fclose(mapping_file);
	return mapping_pairs;
}

void PrintRow(Row tuple) {
	for (auto field : tuple)
		cout << field.str << "\t|";
	cout << endl;
}
int main(int argc, char **argv) {
	google::SetUsageMessage(searchUsageText);
	google::ParseCommandLineFlags(&argc, &argv, true);

	string table_file_name = "dataset/dblp.table";
	string search_file_name = "dataset/dblp.query";
	string max_threshold_file_name = "dataset/rule_max_threshold";
	vector<Similarity> max_thresholds;;

	if (argc >= 2)
		max_threshold_file_name = argv[1];
	if (argc >= 3)
		table_file_name = argv[2];
	if (argc >= 4)
		search_file_name = argv[3];

	double time = getTimeStamp();
	try {
		max_thresholds = loadMapping(max_threshold_file_name);
		loadDataTable(table_file_name, &table, FLAGS_max_base_table_size);
		loadQueries(search_file_name, &queries, FLAGS_max_query_table_size);
	} catch (char *errorMsg) {
		cerr << errorMsg << endl;
		return -1;
	}
	print_debug("Load table time: %.3fs\n", getTimeStamp() - time);
	time = getTimeStamp();

    vector<pair<RowID, RowID>> sim_pairs;
	SimTable *sim_table = new SimTable();
	sim_table->InitTableForSearch(table, max_thresholds);
	for (auto query : queries) {
		vector<RowID> sim_ids = sim_table->Search(query.row, query.sims);
		for (auto id : sim_ids)
			sim_pairs.push_back(make_pair(query.id, id));
	}

	/*
	 *	Output sim_pairs
	 */
	print_debug("sim_pairs.size() = %d\n", int(sim_pairs.size()));
	//freopen("result","w",stdout);
	for (auto pair : sim_pairs) {
		auto tuple1 = table[pair.first];
		auto tuple2 = queries[pair.second].row;
		if (tuple1[0].str != tuple2[0].str) {
			PrintRow(tuple1);
			PrintRow(tuple2);
			puts("");
		}
	}
	//freopen("/dev/stdout", "w", stdout);

	double delta = getTimeStamp() - time;
	try {
		FILE *stat_file = fopen("stat_file.csv","ap");
        fprintf(stat_file, "exp_version %d : (%.2f, %d)\n", FLAGS_exp_version, delta, (int)sim_pairs.size());
	} catch (char *errorMsg) {
		cerr << errorMsg << endl;
		return -1;
	}
}


