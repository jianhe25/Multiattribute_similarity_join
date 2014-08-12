#pragma once
#include "../core.h"
#include "../common.h"
#include <queue>
#include <unordered_set>
#include "../filter.h"
#include <gflags/gflags.h>

/*DECLARE_int32(smallest_leaf_in_search_tree);*/
/*DECLARE_int32(smallest_cost_in_join_tree);*/

// This structure may use subtree token set,
// but currently we use map<string, Node*>
// Each node has 2 layers
// First layer is length, second layer is prefix tokens
struct Node {
	typedef unordered_map<int, Node*> Prefix2Node;
	int id;
    Prefix2Node children;
	bool hasSubtree;
	vector<int> leafIds;
	vector<int> leafIds2;
	long long cost_;

	// For constructing OPTIMAL_JOIN_TREE
	vector<int> list1;
	vector<int> list2;

	Node();
	~Node();
	/*vector<int> list; // TODO: to be deleted*/
};

typedef enum {
	UNORDERED_JOIN_TREE,
	ORDERED_JOIN_TREE,
	ORDERED_SEARCH_TREE,
	UNORDERED_SEARCH_TREE,
	OPTIMAL_JOIN_TREE,
	FULL_SPLIT_TREE
} TreeType;

class TreeIndex {
	TreeType treeType_;
	vector<int> candidates_; // TODO: Can use big array
	Node *root_;
	Verifier verifier_;
	Table *tablePtr1_;
	Table *tablePtr2_;


	int numEstimatedCandidates_;
	long long treeListSize_;
	long long treeNodeNum_;

	vector<Node*> nodePtrs_;
	vector<Node*> leafPtrs_;
	Node* newNode();

	long long CalcTreeCost(Node *node);

    public:
	vector<Similarity> sims_;

	TreeIndex();
	TreeIndex(TreeType treeType);
    ~TreeIndex();

	void BuildSearchTree(Table &table, const vector<Similarity> &sims);
    void BuildJoinTree(Table &table1,
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

	pair<double,double> EstimateBenifitAndCost(const Similarity &sim);

	vector<int> Intersect2Lists(vector<int> &a, vector<int> &b);
	bool contain(const vector<Similarity> &fullSims, const vector<Similarity> &treeSims);
	// unit : MB
	int memory();
	long long size();
	int debug_count_leaf_;
	int debug_save_;
	double treeEntropy_;
	long long treeCost_;

	static long long SingleBufferSize;
};

class treeComparison
{
	public:
	bool operator() (const TreeIndex *tree1, const TreeIndex *tree2) const
	{
		return tree1->treeEntropy_ > tree2->treeEntropy_;
	}
};
