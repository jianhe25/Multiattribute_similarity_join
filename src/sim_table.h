#pragma once

#include <iostream>
#include "core.h"
#include "filter.h"
#include <gflags/gflags.h>
#include "prefix_index/prefix_index.h"
#include "tree_index/tree_index.h"

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
	Table *tablePtr_; // table_ means rowTable
	vector<vector<Field*>> column_table1_;
	vector<vector<Field*>> column_table2_;
    Verifier verifier_;

	Estimation Estimate(const vector<Field*> &column,
            const Field& query,
            Similarity &sim,
            const vector<int> &ids,
            Filter *filter);
	int startJoinTime_;
	/*
	 * Following method are for experiment purpose, it's flexible component
	 */
	vector<RowID> Search0_NoEstimate(const Row &query_row, vector<Similarity> &sims, vector<int> &candidateIDs);
	vector<RowID> Search1_Estimate(const Row &query_row, vector<Similarity> &sims, vector<int> &candidateIDs);
	vector<RowID> Search2_TuneEstimate(const Row &query_row, vector<Similarity> &sims, vector<int> &candidateIDs);

	vector<PrefixIndex*> indexes_;
	vector<TreeIndex*> treeIndexes_; // Multi index experiment
	TreeIndex* treeIndex_; // Single index experiment

	Similarity ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims);
	TreeIndex* ChooseBestTreeIndex(Row &query_row);
	void InitIndex(Table &table1, Table &table2, vector<Similarity> &sims);

public:
	SimTable();
	~SimTable();
	void InitTableForSearch(Table &table, const vector<Similarity> &sims);
	void Init(Table &table1, Table &table2, const vector<Similarity> &sims);
	vector<pair<RowID, RowID>> Join(Table &table1, Table &table2, vector<Similarity> &sims);
	vector<RowID> JoinSearch(Row &query_row, vector<Similarity> &sims);
	vector<RowID> Search(Row &query_row, vector<Similarity> &sims);
};




