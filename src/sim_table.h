#pragma once

#include <iostream>
#include "core.h"
#include "filter.h"
#include <gflags/gflags.h>
#include "index/index.h"

using namespace std;
DECLARE_int32(exp_version);

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
	int num_col_;
	int num_row_;
	Table table_; // table_ means rowTable
	Table column_table_;
    Verifier verifier_;

	Estimation Estimate(const Column &column,
			const Field& query,
			Similarity &sim,
			const vector<int> &ids,
			Filter *filter);
	int startJoinTime;
	/*
	 * Following method are for experiment purpose, it's flexible component
	 */
	vector<RowID> Search0_NoEstimate(const Row &query_row, vector<Similarity> &sims, vector<int> &candidateIDs);
	vector<RowID> Search1_Estimate(const Row &query_row, vector<Similarity> &sims, vector<int> &candidateIDs);
	vector<RowID> Search2_TuneEstimate(const Row &query_row, vector<Similarity> &sims, vector<int> &candidateIDs);

	vector<SimIndex*> indexes;
	Similarity *ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims);
	void InitIndex(vector<Similarity> &sims);

public:
	SimTable();
	~SimTable();
	SimTable(Table &table);
	void Init(Table &table);
	vector<pair<RowID, RowID>> Join(Table &table1, Table &table2, vector<Similarity> &sims);
	vector<RowID> Search(Row &query_row, vector<Similarity> &sims);
};




