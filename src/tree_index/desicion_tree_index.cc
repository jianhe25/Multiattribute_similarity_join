#include "tree_index.h"
#include <cassert>
#include <algorithm>

int Node::g_node_id = 0;
Node::Node() {
	id = ++g_node_id;
	hasSubtree = false;
	leafIds.clear();
}

TreeIndex::TreeIndex() {
	// Default tree type is UNORDERED_JOIN_TREE
	treeType_ = UNORDERED_JOIN_TREE;
	treeListSize_ = 0;
}

TreeIndex::TreeIndex(TreeType treeType) {
	treeType_ = treeType;
	treeListSize_ = 0;
}

TreeIndex::~TreeIndex() {
    for (int i = 0; i < MAX_COLUMN_NUM; ++i) {
        token_counter_[i].clear();
        token_counter2_[i].clear();
	}
}

void TreeIndex::CalcTF() {
    for (const auto &row : *tablePtr1_) {
        for (const auto &sim : sims_) {
            for (int token : row[sim.colx].tokens) {
				token_counter_[sim.colx][token]++;
			}
		}
	}

    for (auto &row : *tablePtr1_) {
        for (const auto &sim : sims_) {
            std::sort(row[sim.colx].tokens.begin(), row[sim.colx].tokens.end(), CompareTokenByTF(token_counter_[sim.colx]));
        }
	}

	if (this->treeType_ == ORDERED_JOIN_TREE || this->treeType_ == UNORDERED_JOIN_TREE) {
		for (const auto &row : *tablePtr2_) {
			for (const auto &sim : sims_)
				for (int token : row[sim.coly].tokens)
					token_counter2_[sim.coly][token]++;
		}

		for (auto &row : *tablePtr2_) {
			for (const auto &sim : sims_) {
				std::sort(row[sim.coly].tokens.begin(), row[sim.coly].tokens.end(), CompareTokenByTF(token_counter2_[sim.coly]));
			}
		}
	}
}
long long TreeIndex::estimateJoinCost(const vector<int> &ids1,
						    		  const vector<int> &ids2,
									  const Similarity &sim) {
	unordered_map<int, int> index1;
	unordered_map<int, int> index2;
    for (int id : ids1) {
		const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i) {
            index1[ tokens[i] ]++;
        }
	}

	for (int id : ids2) {
		const vector<int> &tokens = (*tablePtr2_)[id][sim.coly].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i) {
            index2[ tokens[i] ]++;
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
	print_debug("splited_cost = %lld\n", splited_cost);
	return splited_cost;
}

double TreeIndex::estimateSearchEntropy(const vector<int> &ids1,
									  	const Similarity &sim) {
	unordered_map<int, int> index;
	long long total_prefixlength = 0;
    for (int id : ids1) {
		const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		total_prefixlength += prefixlength;
		for (int i = 0; i < prefixlength; ++i) {
            index[ tokens[i] ]++;
        }
	}
	print_debug("total_prefixlength = %lld, colx = %d\n", total_prefixlength, sim.colx);
	double splited_entropy = 0.0;
	for (const auto &list_size : index) {
		double p = (double)list_size.second / ids1.size();
		splited_entropy += -p * log(p);
	}
	return splited_entropy;
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
 * This tree has two styles,
 *
 *
 */
void TreeIndex::Build(Table &table1,
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

	root_ = new Node();

    this->CalcTF();

	//print_debug("col %d %d sim.size = %lu\n",this->sims_[0].colx, this->sims_[1].colx, this->sims_.size());
	//freopen("result", "w", stdout);

	if (treeType_ == ORDERED_JOIN_TREE) {
		for (int i = 0; i < int(sims_.size()); ++i) {
			sims_[i].splitedCost = estimateJoinCost(row_ids1, row_ids2, sims_[i]);
		}
		sort(sims_.begin(), sims_.end(), compareBySplitedCost);
		print_debug("after sorted\n");
		for (int i = 0; i < int(sims_.size()); ++i) {
			print_debug("sims_.colx = %d splited_cost = %lld\n", sims_[i].colx, sims_[i].splitedCost);
		}
	}

	if (treeType_ == ORDERED_SEARCH_TREE) {
		for (int i = 0; i < int(sims_.size()); ++i) {
			sims_[i].splitedEntropy = estimateSearchEntropy(row_ids1, sims_[i]);
		}
		sort(sims_.begin(), sims_.end(), compareBySplitedEntropy);
		print_debug("after sorted\n");
		for (int i = 0; i < int(sims_.size()); ++i) {
			print_debug("sims_.colx = %d splited_entropy = %f\n", sims_[i].colx, sims_[i].splitedEntropy);
		}
	}

	cout << "when start build index, root_ = " << root_ << endl;
	if (treeType_ == ORDERED_JOIN_TREE || treeType_ == UNORDERED_JOIN_TREE) {
		BuildJoinIndex(root_,
					   row_ids1,
					   row_ids2,
					   /*depth=*/0);
	}

	if (treeType_ == ORDERED_SEARCH_TREE || treeType_ == UNORDERED_SEARCH_TREE) {
		BuildSearchIndex(root_,
					     row_ids1,
					     /*depth=*/0);
	}

	print_debug("tree_list_size: %d, #node: %d, time: %fs\n", treeListSize_, Node::g_node_id, getTimeStamp() - time);
	cout << "when finish build index, root_ = " << root_ << endl;
    // Sort tokens again by their value instead of TF, so we can verify jaccard later
    for (auto &row : *tablePtr1_) {
        for (const auto &sim : sims_) {
            std::sort(row[sim.colx].tokens.begin(), row[sim.colx].tokens.end());
        }
	}

	if (treeType_ == ORDERED_JOIN_TREE || treeType_ == UNORDERED_JOIN_TREE) {
		// Sort tokens again by their value instead of TF, so we can verify jaccard later
		for (auto &row : *tablePtr2_) {
			for (const auto &sim : sims_) {
				std::sort(row[sim.colx].tokens.begin(), row[sim.colx].tokens.end());
			}
		}
	}
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
	double unsplited_entropy = 1.0;
	if (depth == int(sims_.size()) || ids1.size() < 10) {
		node->hasSubtree = false;
		node->leafIds = ids1;
		treeListSize_ += ids1.size();
		return;
	}

	const Similarity &sim = sims_[depth];
	unordered_map<int, vector<int>> index;
    for (int id : ids1) {
		const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i) {
            vector<int> &list = index[ tokens[i] ];
            if (list.empty() || list.back() != id)
                list.push_back(id);
        }
	}

	double splited_entropy = 0;
	for (const auto &list : index) {
		double p = (double)list.second.size() / ids1.size();
		splited_entropy += -p * log(p);
	}

	if (++debug_count % 10000 == 0)
		print_debug("splited_entropy = %f %d\n",splited_entropy, debug_count);

	if (splited_entropy > unsplited_entropy * 10) {
	//if (true) {
		node->hasSubtree = true;
		for (const auto &list : index) {
			int token = list.first;
			Node *new_child = new Node();
			node->children[token] = new_child;
			BuildSearchIndex(new_child, list.second, depth+1);
		}
	} else {
		//if (++num_unsplited % 1000 == 0)
		//print_debug("!!!!!!!!!!splited_entropy = %f unsplite_id = %d %d\n",splited_entropy, ids1.size(), num_unsplited);
		node->hasSubtree = false;
		node->leafIds = ids1;
		treeListSize_ += ids1.size();
	}
}
void TreeIndex::BuildJoinIndex(Node *node,
							   const vector<int> &ids1,
							   const vector<int> &ids2,
							   int depth) {

	long long unsplited_cost = (long long)ids1.size() * ids2.size();
	if (depth == int(sims_.size()) || unsplited_cost <= 10) {
		node->hasSubtree = false;
		node->leafIds = ids1;
		treeListSize_ += ids1.size();
		return;
	}

	const Similarity &sim = sims_[depth];
	unordered_map<int, vector<int>> index;
	unordered_map<int, vector<int>> index2;
    for (int id : ids1) {
		const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i) {
            vector<int> &list = index[ tokens[i] ];
            if (list.empty() || list.back() != id)
                list.push_back(id);
        }
	}

	long long splited_cost = 0;
	for (int id : ids2) {
		const vector<int> &tokens = (*tablePtr2_)[id][sim.coly].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i) {
			vector<int> &list = index2[ tokens[i] ];
			if (list.empty() || list.back() != id)
				list.push_back(id);
		}
	}

	for (const auto &list : index2) {
		int token = list.first;
		int list1_size = 0;
		const auto it = index.find(token);
		list1_size = (it != index.end())? it->second.size() : 0;
		splited_cost += (long long)list1_size * list.second.size();
	}

	if (++debug_count % 10000 == 0) {
		string relation = (splited_cost < unsplited_cost)? "<" : ">";
		print_debug("splited_cost: %lld %s unsplited_cost: %lld %.3f\n", splited_cost, relation.c_str(), unsplited_cost, splited_cost * 1.0 / unsplited_cost);
	}

	//for (const auto &list : index) {
		//int token = list.first;
		//const auto it = index2.find(token);
		//vector<int> list2;
		//if (it != index2.end()) {
			//list2 = it->second;
		//}
		//Node *new_child = new Node();
		//node->children[token] = new_child;
		//BuildJoinIndex(new_child, list.second, list2, depth+1);
	//}

	if (splited_cost == 0) {
		node->hasSubtree = false;
	} else {
		if (splited_cost < unsplited_cost * 0.5) {
			//if (true) {
			node->hasSubtree = true;
			for (const auto &list : index2) {
				int token = list.first;
				const auto it = index.find(token);
				if (it != index.end()) {
					Node *new_child = new Node();
					node->children[token] = new_child;
					const vector<int> *list1 = &(it->second);
					BuildJoinIndex(new_child, *list1, list.second, depth+1);
				}
			}
		} else {
			//print_debug("!!!!!!!!!splited_cost: %lld %s unsplited_cost: %lld\n", splited_cost, relation.c_str(), unsplited_cost);
			node->hasSubtree = false;
			node->leafIds = ids1;
			treeListSize_ += ids1.size();
		}
	}
}

// TODO: Call CalcOverlap many times
bool TreeIndex::VerifyRow(Row a, Row b) {
	for (const auto &sim : sims_)
		if (!verifier_.filter(a[sim.colx], b[sim.coly], sim)) {
			return false;
		}
	return true;
}

vector<int> TreeIndex::getPrefixList(const Row &row) {
	// Would change vector elements
	candidates_.clear();
    debug_count_leaf_ = 0;
    debug_save_ = 0;
	this->TreeSearch(root_, row, 0, /*calcPrefixListSizeOnly=*/false);
	//print_debug("debug_count_leaf_ = %d %d %d\n", debug_count_leaf_, int(candidates_.size()), debug_save_);
	return candidates_;
}

int TreeIndex::calcPrefixListSize(const Row &row) {
	numEstimatedCandidates_ = 0;
    debug_count_leaf_ = 0;
    debug_save_ = 0;
	//print_debug("col %d %d sim.size = %lu\n",sims_[0].colx, sims_[1].colx, sims_.size());
	this->TreeSearch(root_, row, 0, /*calcPrefixListSizeOnly=*/true);
	return numEstimatedCandidates_;
}

void TreeIndex::TreeSearch(const Node *node, const Row &row, int depth, int calcPrefixListSizeOnly) {
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
	std::sort(tokens.begin(), tokens.end(), CompareTokenByTF(token_counter_[sim.colx]));
	int prefix_length = CalcPrefixLength(tokens.size(), sims_[depth]);

	for (int i = 0; i < prefix_length; ++i) {
		const auto it = node->children.find( tokens[i] );
		if (it != node->children.end()) {
			this->TreeSearch(
					it->second,
					row,
					depth+1,
					calcPrefixListSizeOnly);
		}
	}
}

