#ifndef SRC_SIM_TABLE
#define SRC_SIM_TABLE

#include <iostream>
#include "core.h"
#include "filter.h"
#include <gflags/gflags.h>
using namespace std;
DECLARE_int32(exp_version);
typedef int RowID;
struct Estimation {
	double ratio;
	double cost;
	Filter *filter;
	Similarity *sim;
	Estimation(double _ratio, double _cost, Filter *_filter, Similarity *_sim);
	bool operator > (const Estimation &other) const;
	bool operator < (const Estimation &other) const;
};
class SimTable {
	int col_num_;
	int row_num_;
	Table table_; // table_ means rowTable
	Table column_table_;
//	void sortByIDF(TokenColumn *tokenColumn, unordered_map<int, int> *tokenCounter);
//	void buildIndex(Table &table);
	Estimation Estimate(const Column &column,
			const Field& query,
			Similarity &sim,
			const vector<int> &ids,
			Filter *filter);
	int startJoinTime;

	/*
	 * Following method are for experiment purpose, it's flexible component
	 */
	vector<RowID> Search0_NoEstimate(const Row &query_row, vector<Similarity> &sims);
	vector<RowID> Search1_Estimate(const Row &query_row, vector<Similarity> &sims);
	vector<RowID> Search2_TuneEstimate(const Row &query_row, vector<Similarity> &sims);

//	Index ed_indexes_;
// 	Index indexes;
public:
	SimTable();
	SimTable(const Table &table);
	void Init(const Table &table);
	vector<pair<RowID, RowID>> Join(Table &table1, Table &table2, vector<Similarity> &sims);
	vector<RowID> Search(const Row &query_row, vector<Similarity> &sims);
};

#endif // SRC_SIM_TABLE



