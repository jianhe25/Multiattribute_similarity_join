#pragma once
#include "../core.h"
#include "../common.h"
#include <queue>
#include <unordered_set>
#include "../filter.h"

// This structure may use subtree token set,
// but currently we use map<string, Node*>
// Each node has 2 layers
// First layer is length, second layer is prefix tokens
struct Node {
	typedef unordered_map<int, Node*> Prefix2Node;
	int id;
	static int g_node_id;
	vector<Prefix2Node> prefixMaps;
	Node(int max_length);
	bool isLeaf;
	vector<int> leafIds;
};

class TreeIndex {
	/*Table column_table_;*/
	/*vector<bool> is_column1_searched_;*/
	unordered_set<int> candidates_; // TODO: Can use big array
	Node *root_;
	vector<pair<RowID, RowID> > simPairs_;
	Verifier verifier_;
	Table *tablePtr1_;
	Table *tablePtr2_;
	vector<Similarity> sims_;
	int debug_count_leaf_;

    public:
	TreeIndex(int a);
    vector<pair<RowID, RowID> > Join(Table &table1,
											Table &table2,
											vector<Similarity> &sims);
    void ShowIndex(Node *root);
    void BuildIndex(Node *node,
                    const vector<Similarity> &sims,
                    const vector<int> &ids1,
                    int depth);

	vector<pair<RowID, RowID> > Search(const Row &row);
	void TreeSearch(Node *node, const vector<Field> &row, int depth);
	bool VerifyRow(Row a, Row b);
	/*
    int LengthHistogramEstimate(Column &column1,
                                Column &column2,
                                const vector<int>& ids1,
                                const vector<int>& ids2) {
        for (int id : ids1) {
            ++length_bucket1_size_[ column1[id].str.size() ];
        }
        for (int id : ids1) {
            ++length_bucket2_size_[ column2[id].str.size() ];
        }
        // TODO: change multiplication to len : [len-x, len+x]
        int cost = 0;
        for (int l = 0; l < MAX_LENGTH; ++l) {
            cost += length_bucket1_size_[l] * length_bucket2_size_[l];
        }
        return cost;
    }
    */
};

