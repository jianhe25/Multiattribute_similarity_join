#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <algorithm>
using namespace std;

DEFINE_int32(exp_version, 0, "experiment version");

SimTable::SimTable() {}
SimTable::SimTable(const Table &table) {
	Init(table);
}
void SimTable::Init(const Table &table) {
	initFilters();
	table_ = table;
	row_num_ = table_.size();
	col_num_ = table_[0].size();
	cout << "start transpose !" << endl;
	transpose(table_, &column_table_);
	cout << "transpose successfully!" << endl;
/*
	tokenCounter = new unordered_map<int, int>[colNum];
	TokenColumn *tokenTable = new TokenColumn[colNum];
	for (int col = 0; col < colNum; ++col) {
		generateTokens(columnTable[col], similarities[col].distType, &tokenTable[col]);
		sortByIDF(&tokenTable[col], &tokenCounter[col]);
	}
*/
}
vector<pair<RowID, RowID>> SimTable::Join(Table &table1, Table &table2, vector<Similarity> &sims) {
	Init(table1);
	vector<pair<RowID, RowID>> simPairs;;
	startJoinTime = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
		cout << "search " << i << endl;
		vector<RowID> results = Search(table2[i], sims);
		for (int id : results)
			simPairs.push_back(make_pair(id, i));
	}
	return simPairs;
}
vector<RowID> SimTable::Search(const Row &query_row, vector<Similarity> &sims) {
	vector<RowID> result;
	switch (FLAGS_exp_version) {
		case 0 : { result = Search0_NoEstimate(query_row, sims); break; }
		case 1 : { result = Search1_Estimate(query_row, sims); break; }
		case 2 : { result = Search2_TuneEstimate(query_row, sims); break; }
		default: {
			cerr << "Error: NonExist exp_version, exp_version is in [" << 0 << ", " << 2 << "]" << endl;
			result = vector<RowID>(0);
		}
	}
	PrintTime(getTimeStamp() - startJoinTime);
	return result;
}
Estimation SimTable::Estimate(const Column &column,
							const Field &query,
							Similarity &sim,
							const vector<int> &ids) {
	vector<int> sampleIDs;
	for (int i = 0; i < ids.size() * SAMPLE_RATIO; ++i)
		sampleIDs.push_back(ids[rand() % ids.size()]);
	double max_ratio_cost = 0.0;
	double best_ratio = 0.0;
	double best_cost = 1.0; // if cost is zero, then ratio_cost would be nan.
	Filter *best_filter = NULL;
	for (auto filter : g_filters) {
		int pass = 0;
		int startTime = getTimeStamp();
		for (int id : sampleIDs)
			pass += filter->filter(column[id], query, sim);
		double cost = double(getTimeStamp() - startTime) / sampleIDs.size();
		double ratio = 1.0 - double(pass) / sampleIDs.size();

		if ((cost > 0 && ratio / cost > max_ratio_cost) || (cost == 0 && ratio > 0)) {
			max_ratio_cost = ratio / cost;
			best_filter = filter;
			best_ratio = ratio;
			best_cost = cost;
		}
	}
//	printf("ratio = %f, cost = %f, filter = %s\n",best_ratio, best_cost, best_filter->EchoType().c_str());
	Estimation estimation;
	estimation.ratio = best_ratio;
	estimation.cost = best_cost;
	estimation.filter = best_filter;
	estimation.sim = &sim;
	return estimation;
}
/*
 * Sort only by IDF of column1
 */
/*
void SimTable::sortByIDF(TokenColumn *tokenColumn, unordered_map<int, int> *tokenCounter) {
	tokenCounter.clear();
	for (auto field : *tokenColumn)
		for (auto token : field)
			*tokenCounter[token]++;

	for (auto field : *tokenColumn) {
		sort(field.begin(), field.end(), [](int tokenA, int tokenB) {
			return *tokenCounter[tokenA] < *tokenCounter[tokenB];
		});
	}
}
*/
