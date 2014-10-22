#include "tree_index.h"
#include <cassert>
#include <algorithm>

static double GlobalAveragePrefixLength[MAX_COLUMN_NUM];
DEFINE_int32(smallest_leaf_in_search_tree, 10, "if list size in node is smaller than 10, no need split");
DEFINE_int32(smallest_cost_in_join_tree, 10000, "if cost in node is smaller than 1000, no need split");

long long TreeIndex::SingleBufferSize = 0;

template <typename T>
void FreeContainer(T &t) {
	T temp;
	t.swap(temp);
}

Node::Node() {
	hasSubtree = false;
	leafIds.clear();
}

Node::~Node() {
	FreeContainer(list1);
	FreeContainer(list2);
	FreeContainer(leafIds);
	FreeContainer(leafIds2);
	FreeContainer(children);
}

TreeIndex::TreeIndex() {
	// Default tree type is UNORDERED_JOIN_TREE
	TreeIndex(UNORDERED_JOIN_TREE);
}

TreeIndex::TreeIndex(TreeType treeType) {
	treeType_ = treeType;
	treeListSize_ = 0;
	treeEntropy_ = 0;
	treeNodeNum_ = 0;
}

TreeIndex::~TreeIndex() {
}

long long TreeIndex::estimateJoinCost(const vector<int> &ids1,
						    		  const vector<int> &ids2,
									  const Similarity &sim) {
	unordered_map<int, int> index1;
	unordered_map<int, int> index2;
    for (int id : ids1) {
		Field &field = (*tablePtr1_)[id][sim.colx];
		const vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(field, sim);
		if (prefixlength > 0) {
			for (int i = 0; i < prefixlength; ++i) {
				index1[ tokens[i] ]++;
			}
		} else {
			index1[-1]++;
		}
	}

	for (int id : ids2) {
		Field &field = (*tablePtr2_)[id][sim.coly];
		const vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(field, sim);
		if (prefixlength > 0) {
			for (int i = 0; i < prefixlength; ++i) {
				index2[ tokens[i] ]++;
			}
		} else {
			index2[-1]++;
		}
	}

	long long splited_cost = 0;
    for (const auto &list1 : index1) {
        int token = list1.first;
		int list2_size = 0;
		const auto it = index2.find(token);
		if (it != index2.end())
			list2_size = it->second;
		else
			list2_size = 0;
		splited_cost += (long long)list2_size * list1.second;
	}
	//print_debug("splited_cost = %lld\n", splited_cost);
	return splited_cost;
}

// TODO: fix with new entropy model!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Cannot use
double TreeIndex::estimateSearchEntropy(const vector<int> &ids1,
									  	const Similarity &sim) {
	return 0.0;
	//unordered_map<int, int> index;
	//long long total_prefixlength = 0;
    //for (int id : ids1) {
		//const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		//int prefixlength = CalcPrefixLength(tokens.size(), sim);
		//if (prefixlength > 0) {
			//for (int i = 0; i < prefixlength; ++i) {
				//index[ tokens[i] ]++;
			//}
		//} else {
			//index[-1]++;
		//}
		//total_prefixlength += prefixlength;
	//}
	//print_debug("total_prefixlength = %lld, colx = %d\n", total_prefixlength, sim.colx);
	//double splited_entropy = 0.0;
	//for (const auto &list_size : index) {
		//double p = (double)list_size.second / ids1.size();
		//splited_entropy += -p * log(p);
	//}
	//double avg_prefixlength = double(total_prefixlength) / ids1.size();
	//return splited_entropy - avg_prefixlength;
}

bool compareBySplitedCost(const Similarity &sim1,
						  const Similarity &sim2) {
	return sim1.splitedCost < sim2.splitedCost;
}
bool compareBySplitedEntropy(const Similarity &sim1,
						     const Similarity &sim2) {
	return sim1.splitedEntropy > sim2.splitedEntropy;
}

/*
 * Built with one tables
 */
void TreeIndex::BuildSearchTree(Table &table,
							    const vector<Similarity> &sims) {

	for (unsigned j = 0; j < sims.size(); ++j) {
		int colx = sims[j].colx;
		// not initialize average prefix length
		if (GlobalAveragePrefixLength[colx] == 0) {
			for (unsigned i = 0; i < table.size(); ++i) {
				GlobalAveragePrefixLength[colx] += CalcPrefixLength(table[i][colx], sims[j]);
			}
			GlobalAveragePrefixLength[colx] /= table.size();
			print_debug("average %d : %f\n", colx, GlobalAveragePrefixLength[colx]);
		}
	}

	double time = getTimeStamp();
	sims_ = sims;
	vector<int> row_ids(table.size(), 0);
	tablePtr1_ = &table;
	for (unsigned id = 0; id < table.size(); ++id)
		row_ids[id] = id;
	root_ = newNode();
	if (treeType_ == ORDERED_SEARCH_TREE) {
		for (int i = 0; i < int(sims_.size()); ++i) {
			sims_[i].splitedEntropy = estimateSearchEntropy(row_ids, sims_[i]);
		}
		sort(sims_.begin(), sims_.end(), compareBySplitedEntropy);
	}
	BuildSearchIndex(root_,
					 row_ids,
					 /*depth=*/0);

	print_debug("Build search tree time %fs, entropy : %f\n", getTimeStamp() - time, treeEntropy_);
	print_debug("sim : ");
	for (const auto &sim : sims_)
		printf("%d ",sim.colx);
	puts("");
}

/*
 * Build tree with two tables,
 */
void TreeIndex::BuildJoinTree(Table &table1,
						      Table &table2,
							  const vector<Similarity> &sims) {
	double time = getTimeStamp();
	sims_ = sims;
	tablePtr1_ = &table1;
	tablePtr2_ = &table2;

	vector<int> row_ids1(table1.size(), 0);
	vector<int> row_ids2(table2.size(), 0);
	for (unsigned id = 0; id < table1.size(); ++id)
		row_ids1[id] = id;
	for (unsigned id = 0; id < table2.size(); ++id)
		row_ids2[id] = id;

	root_ = newNode();
	if (treeType_ == ORDERED_JOIN_TREE || treeType_ == OPTIMAL_JOIN_TREE) {
		for (int i = 0; i < int(sims_.size()); ++i) {
			sims_[i].splitedCost = estimateJoinCost(row_ids1, row_ids2, sims_[i]);
		}
		sort(sims_.begin(), sims_.end(), compareBySplitedCost);
		for (int i = 0; i < int(sims_.size()); ++i) {
			print_debug("sims_.colx = %d splited_cost = %lld\n", sims_[i].colx, sims_[i].splitedCost);
		}
	}

	if (treeType_ == ORDERED_JOIN_TREE || treeType_ == UNORDERED_JOIN_TREE || treeType_ == OPTIMAL_JOIN_TREE || treeType_ == ITERATIVE_ORDERED_JOIN_TREE) {
		BuildJoinIndex(root_,
					   row_ids1,
					   row_ids2,
					   /*depth=*/0);
	}

	long long old_tree_cost = CalcTreeCost(root_);
	if (treeType_ == OPTIMAL_JOIN_TREE) {
		for (int i = nodePtrs_.size() - 1; i >= 0; --i) {
			Node *node = nodePtrs_[i];
			if (node->hasSubtree) {
				long long subtree_cost = 0;
				for (const auto pair : node->children) {
					subtree_cost += pair.second->cost_;
				}
				long long merge_cost = (long long)node->list1.size() * node->list2.size();
				//print_debug("subtree_cost: %lld, merge_cost: %lld\n", subtree_cost, merge_cost);
				if (merge_cost < subtree_cost) {
					node->hasSubtree = false;
					node->leafIds = node->list1;
					node->leafIds2 = node->list2;
					for (auto &pair : node->children) {
						delete pair.second;
					}
					node->children.clear();
					node->cost_ = merge_cost;
					//print_debug("remove unnessary nodes at %d, where subtree_cost: %lld, merge_cost: %lld\n", node->id, subtree_cost, node->cost_);
				} else {
					node->cost_ = subtree_cost;
				}
			} else {
				node->cost_ = (long long)node->list1.size() * node->list2.size();
			}
		}
	}
	//treeCost_ = CalcTreeCost(root_);

	print_debug("tree_list_size: %lld, #node: %ld, time: %fs, joinCost: %lld, old_tree_cost: %lld\n", treeListSize_, nodePtrs_.size(), getTimeStamp() - time, treeCost_, old_tree_cost);

}

bool CompareListPair(const pair<int, vector<int>> &list1,
                 const pair<int, vector<int>> &list2) {
    return list1.second.size() > list2.second.size();
}

int debug_count = 0;
int num_unsplited = 0;
void TreeIndex::BuildSearchIndex(Node *node,
							     const vector<int> &ids1,
							     int depth) {
	if (depth == int(sims_.size()) || ids1.size() < FLAGS_smallest_leaf_in_search_tree) {
		node->hasSubtree = false;
		node->leafIds = ids1;
		treeListSize_ += ids1.size();
		leafPtrs_.push_back(node);
		return;
	}

	int total_prefixlength = 0;
	const Similarity &sim = sims_[depth];
	unordered_map<int, vector<int>> index;
    for (int id : ids1) {
		Field &field = (*tablePtr1_)[id][sim.colx];
		const vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(field, sim);
		if (prefixlength > 0) {
			for (int i = 0; i < prefixlength; ++i) {
				vector<int> &list = index[ tokens[i] ];
				if (list.empty() || list.back() != id)
					list.push_back(id);
			}
		} else {
			assert(0);
			index[-1].push_back(id);
		}
		total_prefixlength += prefixlength;
	}
	//double splited_entropy = 0;
	//double avg_prefixlength = double(total_prefixlength) / ids1.size();
	//for (const auto &list : index) {
		//double p = double(list.second.size()) / ;
		//splited_entropy += -p * log(p);
		////print_debug("list.size() = %lld, ids1.size() = %lld, splited_entropy = %f \n", list.second.size(), ids1.size(), splited_entropy);
	//}
	//splited_entropy /= avg_prefixlength;

	double num_tuples_layer = tablePtr1_->size();
	for (int i = 0; i < depth; ++i) {
		num_tuples_layer *= GlobalAveragePrefixLength[sims_[i].colx];
	}
	double unsplited_entropy = 0.0; // depth == 0, -1.0 * log(1.0)
	if (depth > 0) {
		double p = double(ids1.size()) / (num_tuples_layer / GlobalAveragePrefixLength[sims_[depth-1].colx]);
		unsplited_entropy = -p * log(p);
	}

	double splited_entropy = 0.0;
	for (const auto &list : index) {
		double p = double(list.second.size()) / num_tuples_layer;
		splited_entropy += -p * log(p);
		//print_debug("p %f %f\n",p, -p * log(p));
	}
	splited_entropy *= 1.5;

	if (splited_entropy > unsplited_entropy) {
		//if (++debug_count % 10000 == 0)
		//print_debug("splited_entropy = %f unsplited_entropy = %f, %d\n",splited_entropy, unsplited_entropy, debug_count);
		treeEntropy_ += splited_entropy - unsplited_entropy;
		//print_debug("splited_entropy = %f, unsplited_entropy = %f, treeEntropy_ = %f\n", splited_entropy, unsplited_entropy, treeEntropy_);
		node->hasSubtree = true;
		for (const auto &list : index) {
			int token = list.first;
			Node *new_child = newNode();
			node->children[token] = new_child;
			BuildSearchIndex(new_child, list.second, depth+1);
		}
	} else {
		//if (++num_unsplited % 10000 == 0)
		//print_debug("!!!!!!!!!!splited_entropy = %f %f unsplite_id = %ld %d\n",splited_entropy, unsplited_entropy, ids1.size(), num_unsplited);
		node->hasSubtree = false;
		node->leafIds = ids1;
		treeListSize_ += ids1.size();
		leafPtrs_.push_back(node);
	}
}

Node* TreeIndex::newNode() {
	++treeNodeNum_;
	Node* node = new Node();
	//nodePtrs_.push_back(node);
	node->id = treeNodeNum_;
	return node;
}

void TreeIndex::BuildJoinIndex(Node *node,
							   const vector<int> &ids1,
							   const vector<int> &ids2,
							   int depth) {

	if (treeType_ == OPTIMAL_JOIN_TREE) {
		node->list1 = ids1;
		node->list2 = ids2;
	}

	long long unsplited_cost = (long long)ids1.size() * ids2.size();
	if (depth == int(sims_.size()) || unsplited_cost < FLAGS_smallest_cost_in_join_tree) {
		node->hasSubtree = false;
		treeListSize_ += ids1.size();
		node->leafIds = ids1;
		//node->leafIds2 = ids2;
		return;
	}
	//if (node->id % 1000000 == 0)
		//print_debug("node = %d, depth = %d %lu %lu\n", node->id, depth, ids1.size(), ids2.size());

	const Similarity &sim = sims_[depth];
	unordered_map<int, vector<int>> index;
	unordered_map<int, vector<int>> index2;
    for (int id : ids1) {
		Field &field = (*tablePtr1_)[id][sim.colx];
		const vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(field, sim);
		for (int i = 0; i < prefixlength; ++i) {
			vector<int> &list = index[ tokens[i] ];
			if (list.empty() || list.back() != id)
				list.push_back(id);
		}
	}

	long long splited_cost = 0;
	for (int id : ids2) {
		Field &field = (*tablePtr2_)[id][sim.coly];
		const vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(field, sim);
		for (int i = 0; i < prefixlength; ++i) {
			vector<int> &list = index2[ tokens[i] ];
			if (list.empty() || list.back() != id)
				list.push_back(id);
		}
	}

	for (const auto &list2 : index2) {
		int token = list2.first;
		int list1_size = 0;
		const auto it = index.find(token);
		list1_size = (it != index.end())? it->second.size() : 0;
		splited_cost += (long long)list1_size * list2.second.size();
	}

	if (splited_cost == 0) {
		node->hasSubtree = false;
		node->leafIds.clear();
		//node->leafIds2.clear();
		return;
	}
	if (splited_cost < unsplited_cost || treeType_ == OPTIMAL_JOIN_TREE) {
		node->hasSubtree = true;
		for (auto &list2 : index2) {
			int token = list2.first;
			const auto it = index.find(token);
			if (it != index.end()) {
				Node *new_child = newNode();
				node->children[token] = new_child;
				vector<int> *list1 = &(it->second);
				BuildJoinIndex(new_child, *list1, list2.second, depth+1);
				FreeContainer(*list1);
			}
			FreeContainer(list2.second);
		}
	} else {
		//print_debug("!!!!!!!!!splited_cost: %lld unsplited_cost: %lld, depth: %d\n", splited_cost, unsplited_cost, depth);
		node->hasSubtree = false;
		node->leafIds = ids1;
		//node->leafIds2 = ids2;
		treeListSize_ += ids1.size();
	}
}

vector<int> TreeIndex::getPrefixList(Row &row) {
	// Would change vector elements
	candidates_.clear();
    debug_count_leaf_ = 0;
    debug_save_ = 0;
	this->TreeSearch(root_, row, 0, /*calcPrefixListSizeOnly=*/false);
	//print_debug("debug_count_leaf_ = %d %d %d\n", debug_count_leaf_, int(candidates_.size()), debug_save_);
	return candidates_;
}

int TreeIndex::calcPrefixListSize(Row &row) {
	numEstimatedCandidates_ = 0;
    debug_count_leaf_ = 0;
    debug_save_ = 0;
	this->TreeSearch(root_, row, 0, /*calcPrefixListSizeOnly=*/true);
	return numEstimatedCandidates_;
}

void TreeIndex::TreeSearch(const Node *node, Row &row, int depth, int calcPrefixListSizeOnly) {
	//print_debug("depth = %d, node = %d, hasSubtree: %d\n",depth, node->id, node->hasSubtree);
	if (!node->hasSubtree) {
		debug_count_leaf_ += node->leafIds.size();
		if (calcPrefixListSizeOnly) {
			numEstimatedCandidates_ += node->leafIds.size();
		} else {
			for (int id : node->leafIds) {
				candidates_.push_back(id);
			}
		}
		return;
	}
    if (node != root_ && node->hasSubtree) {
        debug_save_ += node->leafIds.size();
    }

    const auto &sim = sims_[depth];
	vector<int> tokens = row[sim.coly].tokens;
	int prefix_length = CalcPrefixLength(row[sim.coly], sims_[depth]);

	for (int i = 0; i < prefix_length; ++i) {
		const auto it = node->children.find( tokens[i] );
		//print_debug("tokens = %d\n", tokens[i]);
		if (it != node->children.end()) {
			this->TreeSearch(
					it->second,
					row,
					depth+1,
					calcPrefixListSizeOnly);
		} else {
			//print_debug("token = %d\n", tokens[i]);
			//printRow(row);
			//assert(0);
			//print_debug("tokens not found\n");
		}
	}
}

/*
 * tree memory: estimated memory of tree
 * #node * size(node) + #map_usage + #list_size
 * unit: MB
 */
int TreeIndex::memory() {
	const static int MB = 1024 * 1024; // byte
	const static int MAP_ELEMENT_SIZE = 100;
	int memoryMB = (treeNodeNum_ * (sizeof(Node)+ MAP_ELEMENT_SIZE) + treeListSize_) / MB;
	return memoryMB;
}

long long TreeIndex::size() {
	return treeListSize_;
}

long long TreeIndex::CalcTreeCost(Node *node) {
	long long tree_cost = 0;
	if (node->hasSubtree) {
		for (const auto &pair : node->children)
			tree_cost += CalcTreeCost(pair.second);
	} else {
		tree_cost = node->leafIds.size() * node->leafIds2.size();
	}
	return tree_cost;
}

pair<double,double> TreeIndex::EstimateBenifitAndCost(const Similarity &sim) {
	double num_tuples_layer = tablePtr1_->size();
	int depth = sims_.size();
	for (int i = 0; i < depth; ++i) {
		num_tuples_layer *= GlobalAveragePrefixLength[sims_[i].colx];
	}
	num_tuples_layer *= GlobalAveragePrefixLength[sim.colx];

	double entropy_gain = 0.0;
	double space_cost = 0.0;
	double DOWN_SAMPLE_RATIO = 0.1;
	int num_test_leaves = leafPtrs_.size() * DOWN_SAMPLE_RATIO;
	for (int i = 0; i < num_test_leaves; ++i) {
		//for (const Node *node : leafPtrs_)
		const Node *node = leafPtrs_[rand() * rand() % leafPtrs_.size()];
		if (node->leafIds.size() > FLAGS_smallest_leaf_in_search_tree) {
			const vector<int> &ids1 = node->leafIds;
			double unsplited_entropy = 0.0; // depth == 0, -1.0 * log(1.0)
			if (depth > 0) {
				double p = double(ids1.size()) / (num_tuples_layer / GlobalAveragePrefixLength[sims_[depth-1].colx]);
				unsplited_entropy = -p * log(p);
			}

			long long total_prefixlength = 0;
			unordered_map<int, vector<int>> index;
			for (int id : ids1) {
				Field &field = (*tablePtr1_)[id][sim.colx];
				const vector<int> &tokens = field.tokens;
				int prefixlength = CalcPrefixLength(field, sim);
				if (prefixlength > 0) {
					for (int i = 0; i < prefixlength; ++i) {
						vector<int> &list = index[ tokens[i] ];
						if (list.empty() || list.back() != id)
							list.push_back(id);
					}
				}
				total_prefixlength += prefixlength;
			}
			double splited_entropy = 0.0;
			for (const auto &list : index) {
				double p = double(list.second.size()) / num_tuples_layer;
				splited_entropy += -p * log(p);
				//print_debug("p %f %f\n",p, -p * log(p));
			}
			splited_entropy *= 1.5;
			if (splited_entropy > unsplited_entropy) {
				entropy_gain += splited_entropy - unsplited_entropy;
				space_cost += total_prefixlength;
			}
		}
	}
	space_cost /= TreeIndex::SingleBufferSize;
	entropy_gain /= DOWN_SAMPLE_RATIO;
	space_cost /= DOWN_SAMPLE_RATIO;
	print_debug("tree_sim: %d, expand_sim %d, benefit: %f, cost: %f ratio: %f\n", sims_[0].colx, sim.colx, entropy_gain, space_cost, entropy_gain / space_cost);
	return make_pair(entropy_gain, space_cost);
}


