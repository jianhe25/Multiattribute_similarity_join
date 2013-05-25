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
//	typedef enum {EXACT, HYBRID} Type;
//	Type type;
	Filter *filter;
	Similarity *sim;
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
			const vector<int> &ids);
	int startJoinTime;

	/*
	 * Following method are for experiment purpose, it's flexible component
	 */
	vector<RowID> Search0_NoEstimate(const Row &query_row, vector<Similarity> &sims);
	vector<RowID> Search1_Estimate(const Row &query_row, vector<Similarity> &sims);
	vector<RowID> Search2_TuneEstimate(const Row &query_row, vector<Similarity> &sims);
public:
	SimTable();
	SimTable(const Table &table);
	void Init(const Table &table);
	vector<pair<RowID, RowID>> Join(Table &table1, Table &table2, vector<Similarity> &sims);
	vector<RowID> Search(const Row &query_row, vector<Similarity> &sims);
};

#endif // SRC_SIM_TABLE



