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
class CompareTreeByEntropy
{
	public:
	CompareTreeByEntropy() {
	}
	bool operator() (const TreeIndex *tree1, const TreeIndex *tree2) const
	{
		return tree1->treeEntropy_ > tree2->treeEntropy_;
	}
} ;
class SimTable {
	int num_col_;
	int num_row_;
	Table *tablePtr_; // table_ means rowTable
	vector<vector<Field*>> column_table1_;
	vector<vector<Field*>> column_table2_;
    Verifier verifier_;
	long long numCandidatePairs_;
	long long numRepeatCandidatePairs_;

	Estimation Estimate(const Table &table,
            const Field& query,
            Similarity &sim,
            const vector<int> &ids,
            Filter *filter);
	int startJoinTime_;
	CompareTreeByEntropy compareTreeByEntropyObject;
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
	void InitJoinIndex(Table &table1, Table &table2, vector<Similarity> &sims);
	void CopyColumnY(vector<Similarity> *treeSims, const vector<Similarity> &querySims);
	vector<RowID> JoinSearch(Row &query_row, vector<Similarity> &sims);
	void InitJoin(Table &table1, Table &table2, const vector<Similarity> &sims);
	bool contain(const vector<Similarity> &fullSims, const vector<Similarity> &treeSims);
	vector<int> Intersect2Lists(vector<int> &a, vector<int> &b);
	TreeIndex* Concat2Trees(TreeIndex *tree1, TreeIndex *tree2);
public:
	SimTable();
	~SimTable();
	vector<pair<RowID, RowID>> Join(Table &table1, Table &table2, vector<Similarity> &sims);
	vector<RowID> Search(Row &query_row, vector<Similarity> &sims);
	void InitSearch(Table &table, const vector<Similarity> &sims);
};


