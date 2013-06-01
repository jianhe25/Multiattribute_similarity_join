#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <algorithm>
using namespace std;

DEFINE_int32(exp_version, 0, "experiment version");

Estimation::Estimation(double _ratio, double _cost, Filter *_filter, Similarity *_sim) :
	ratio(_ratio), cost(_cost), filter(_filter), sim(_sim) {
}
bool Estimation::operator > (const Estimation &other) const {
	return this->ratio * other.cost < this->cost * other.ratio;
	// aka. ratio / cost < other.ratio / other.cost
}
bool Estimation::operator < (const Estimation &other) const {
	return !(*this > other);
}

SimTable::SimTable() {}
SimTable::SimTable(const Table &table) {
	Init(table);
}
void SimTable::Init(const Table &table) {
	initFilters();
	table_ = table;
	row_num_ = table_.size();
	col_num_ = table_[0].size();
//	*ed_indexes_ = new Index[MAX_COLUMN_NUMBER];
//	*jc_indexes_ = new Index[MAX_COLUMN_NUMBER];
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
/*
void SimTable::InitIndexes(Similarity &sims) {
	for (auto &sim : sims) {
		ed_index[sim.colX].build();
	}
}
*/
vector<pair<RowID, RowID>> SimTable::Join(Table &table1, Table &table2, vector<Similarity> &sims) {
	Init(table1);
//	InitIndexes(sims);
	vector<pair<RowID, RowID>> simPairs;;
	startJoinTime = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
	//	cout << "search " << i << endl;
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
//	PrintTime(getTimeStamp() - startJoinTime);
	return result;
}
Estimation SimTable::Estimate(const Column &column,
							const Field &query,
							Similarity &sim,
							const vector<int> &ids,
							Filter *filter) {
	static vector<int> sampleIDs;
	if (sampleIDs.empty()) {
		for (int i = 0; i < ids.size() * SAMPLE_RATIO; ++i)
			sampleIDs.push_back(ids[rand() % ids.size()]);
	}
	int pass = 0;
	int startTime = getTimeStamp();
	for (int id : sampleIDs)
		pass += filter->filter(column[id], query, sim);
	double cost = double(getTimeStamp() - startTime) / sampleIDs.size();
	double ratio = 1.0 - double(pass) / sampleIDs.size();
	return Estimation(ratio, cost, filter, &sim);
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
