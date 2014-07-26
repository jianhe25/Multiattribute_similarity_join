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
	/*vector<int> list; // TODO: to be deleted*/
};

typedef enum {
	UNORDERED_JOIN_TREE,
	ORDERED_JOIN_TREE,
	ORDERED_SEARCH_TREE,
	UNORDERED_SEARCH_TREE,
	FULL_SPLIT_TREE
} TreeType;

class TreeIndex {
	TreeType treeType_;
	/*Table column_table_;*/
	/*vector<bool> is_column1_searched_;*/
	vector<int> candidates_; // TODO: Can use big array
	Node *root_;
	Verifier verifier_;
	Table *tablePtr1_;
	Table *tablePtr2_;
    unordered_map<int,int> token_counter_[MAX_COLUMN_NUM]; // MAX COLUMN NUMBER = 50
    unordered_map<int,int> token_counter2_[MAX_COLUMN_NUM]; // MAX COLUMN NUMBER = 50
	int numEstimatedCandidates_;
	int treeListSize_;

    public:

	vector<Similarity> sims_;

	TreeIndex();
	TreeIndex(TreeType treeType);
    ~TreeIndex();

    void CalcTF();

    void Build(Table &table1,
			   Table &table2,
			   const vector<Similarity> &sims);

    void BuildJoinIndex(Node *node,
						const vector<int> &ids1,
						const vector<int> &ids2,
						int depth);

    void BuildSearchIndex(Node *node,
					      const vector<int> &ids1,
						  int depth);

	vector<int> getPrefixList(const Row &row);
	void TreeSearch(const Node *node, const vector<Field> &row, int depth, int CalcPrefixListSizeOnly);
	int calcPrefixListSize(const Row &row);
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

	long long estimateJoinCost(const vector<int> &ids1,
							   const vector<int> &ids2,
							   const Similarity &sim);

	double estimateSearchEntropy(const vector<int> &ids1,
								 const Similarity &sim);

	int debug_count_leaf_;
	int debug_save_;
};

