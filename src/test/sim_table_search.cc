#include <gflags/gflags.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include "../common.h"
#include "../sim_table.h"
#include "../tree_index/tree_index.h"
#include "help_text.h"
#include <algorithm>

using namespace std;

const int MAX_LINE_LENGTH = 10000;

DEFINE_int32(max_base_table_size, 1000, "max tuple number in table1");
DEFINE_int32(max_query_table_size, 1000, "max tuple number in table2");

int columnNum;
Table table;
vector<Query> queries;

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
	int col1, col2;
	double dist;
	char operand[100];
	int num_sim;
	int queryid;
	while (fgets(line, MAX_LINE_LENGTH, query_file))
	if (line[0] == '#') {
		Query query;
		vector<string> strs;
		sscanf(line, "#query %d %d", &queryid, &num_sim);
		queryid--;

		/* read query row */
		//print_debug("num_sim = %d\n", num_sim);
		if (fgets(line, MAX_LINE_LENGTH, query_file) == NULL)
			print_debug("error: fgets line");
		//print_debug("line = %s\n",line);
		splitString(line, '|', strs);
		int num_col = strs.size();
		// Validate column_number
		if (num_col != -1 && (int)strs.size() != num_col) {
			cerr << "Warning : table row has different column number" << " tuple_number = " << table.size() << endl;
			continue;
		}
		for (auto &word : strs) {
			stripString(word);
			query.row.push_back(Field(word, queryid));
			query.id = queryid;
		}

		bool has_error = false;
		/* read Similarity */
		for (int i = 0; i < num_sim; ++i) {
			if (fgets(line, MAX_LINE_LENGTH, query_file) == NULL)
				print_debug("error: fgets line");
			sscanf(line, "%s %d %d %lf", operand, &col1, &col2, &dist);
			if (getSimType(operand) == NON_DEFINE) {
				print_debug("NonExist Similarity Function %s\n", operand);
				print_debug("dist = %f %d\n", dist, queryid);
				has_error = true;
				break;
			}
			query.sims.push_back( Similarity(col1, col2, dist, getSimType(operand)) );
		}
		//PrintSims(query.sims);
		if (!has_error)
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
		if (getSimType(operand) == NON_DEFINE) {
			print_debug("NonExist Similarity Function %s\n", operand);
			break;
		}
		mapping_pairs.push_back(Similarity(col1, col1, dist, getSimType(operand)));
	}
	PrintSims(mapping_pairs);
	fclose(mapping_file);
	return mapping_pairs;
}

int main(int argc, char **argv) {
	google::SetUsageMessage(searchUsageText);
	google::ParseCommandLineFlags(&argc, &argv, true);

	string table_file_name = "dataset/dblp.table";
	string search_file_name = "dataset/dblp.query";
	string threshold_lowerbound_file_name = "dataset/rule_threshold_lowerbound";
	vector<Similarity> thresholds_lowerbound;

	if (argc >= 2)
		threshold_lowerbound_file_name = argv[1];
	if (argc >= 3)
		table_file_name = argv[2];
	if (argc >= 4)
		search_file_name = argv[3];

	double time = getTimeStamp();
	try {
		thresholds_lowerbound = loadMapping(threshold_lowerbound_file_name);
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
	sim_table->InitSearch(table, thresholds_lowerbound);
	for (Query &query : queries) {
		for (const auto &sim : query.sims) {
			if (sim.distType == ED || sim.distType == ES) {
				query.row[sim.coly].GenerateGrams();
			} else {
				query.row[sim.coly].GenerateTokens();
			}
			vector<int> &tokens = query.row[sim.coly].tokens;
			const unordered_map<int,int> &id_map = g_id_map[sim.colx];
			for (int i = 0; i < tokens.size(); ++i) {
				const auto it = id_map.find(tokens[i]);
				if (it == id_map.end())
					tokens[i] = -1;
				else
					tokens[i] = it->second;
			}
			sort(tokens.begin(), tokens.end());
		}
	}

	int queryid = 0;
	for (int exp = 4; exp <= 4; ++exp) {
		double tau = 1 - exp * 0.05;
		double start_search_time = getTimeStamp();
		FLAGS_total_filter_time = 0.0;
		for (auto query : queries) {
			for (Similarity &sim : query.sims) {
				sim.dist = tau;
			}
			vector<RowID> sim_ids = sim_table->Search(query.row, query.sims);
			for (auto id : sim_ids)
				sim_pairs.push_back(make_pair(query.id, id));
			//if (sim_ids.size() > 1000) {
				//PrintSims(query.sims);
				//print_debug("result size = %d\n", sim_ids.size());
			//}
			if (++queryid % 10000 == 0) {
				print_debug("queryid %d, sim_size %d time %f\n",queryid, sim_pairs.size(), getTimeStamp() - start_search_time);
			}
		}
		double total_time = getTimeStamp() - start_search_time;
		print_debug("filter_time: %f, verify: %f, total_time : %f\n", FLAGS_total_filter_time, total_time - FLAGS_total_filter_time, total_time);
		ExportTime("filter", FLAGS_total_filter_time);
		ExportTime("total", getTimeStamp() - start_search_time);
	}

	/*
	 *	Output sim_pairs
	 */
	print_debug("sim_pairs.size() = %d\n", int(sim_pairs.size()));
	//freopen("result","w",stdout);
	//for (auto pair : sim_pairs) {
		//print_debug("pair = %d %d\n", pair.first, pair.second);
		//auto row1 = queries[pair.first].row;
		//auto row2 = table[pair.second];
		//if (row1[0].str != row2[0].str) {
			//printRow(row1);
			//printRow(row2);
			//puts("");
		//}
	//}
	//freopen("/dev/stdout", "w", stdout);
}


