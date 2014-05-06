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
    Prefix2Node children;
	Node();
	bool hasSubtree;
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
    unordered_map<int,int> token_counter_[100]; // MAX COLUMN NUMBER = 100

    public:
	TreeIndex();
    ~TreeIndex();

    void CalcTF();
    vector<pair<RowID, RowID> > Join(Table &table1,
									 Table &table2,
									 vector<Similarity> &sims);
    void BuildIndex(Node *node,
                    const vector<int> &ids1,
                    int depth,
                    bool hasSubtree);

	vector<pair<RowID, RowID> > Search(const Row &row);
	void TreeSearch(Node *node, const vector<Field> &row, int depth);
	bool VerifyRow(Row a, Row b);

	struct CompareTokenByTF {
        const unordered_map<int, int> &token_counter;
		CompareTokenByTF(const unordered_map<int,int> &_token_counter) : token_counter(_token_counter) {}
		bool operator()(int a, int b) {
			const auto it1 = token_counter.find(a);
			const auto it2 = token_counter.find(b);
			if (it1 == token_counter.end() && it2 == token_counter.end()) {
				return a < b;
			} else if (it1 == token_counter.end()) {
				return true;
			} else if (it2 == token_counter.end()) {
				return false;
			} else {
				return it1->second < it2->second || (it1->second == it2->second && a < b);
			}
		}
	};

	int debug_count_leaf_;
    int debug_save_;
};

