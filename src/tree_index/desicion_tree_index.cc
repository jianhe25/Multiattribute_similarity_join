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

void TreeIndex::Build(Table &table1,
					  Table &table2,
					  vector<Similarity> &sims) {
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

	BuildIndex(root_,
               row_ids1,
			   row_ids2,
               /*depth=*/0);

    // Sort tokens again by their value instead of TF, so we can verify jaccard later
    for (auto &row : *tablePtr1_) {
        for (const auto &sim : sims_) {
            std::sort(row[sim.colx].tokens.begin(), row[sim.colx].tokens.end());
        }
	}

    // Sort tokens again by their value instead of TF, so we can verify jaccard later
    for (auto &row : *tablePtr2_) {
        for (const auto &sim : sims_) {
            std::sort(row[sim.colx].tokens.begin(), row[sim.colx].tokens.end());
        }
	}
}

bool CompareListPair(const pair<int, vector<int>> &list1,
                 const pair<int, vector<int>> &list2) {
    return list1.second.size() > list2.second.size();
}

void TreeIndex::BuildIndex(Node *node,
                           const vector<int> &ids1,
						   const vector<int> &ids2,
                           int depth) {

	if (depth == sims_.size()) {
		node->hasSubtree = false;
		node->leafIds = ids1;
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

	for (int id : ids2) {
		const vector<int> &tokens = (*tablePtr2_)[id][sim.coly].tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i) {
            vector<int> &list = index2[ tokens[i] ];
            if (list.empty() || list.back() != id)
                list.push_back(id);
        }
	}

	int splited_cost = 0;
    for (const auto &list : index) {
        int token = list.first;
		int list2_size = 0;
		const auto it = index2.find(token);
		if (it != index2.end())
			list2_size = it->second.size();
		else
			list2_size = 0;
		splited_cost += list2_size * list.second.size();
	}

	int unsplited_cost = ids1.size() * ids2.size();
	if (splited_cost < unsplited_cost) {
		node->hasSubtree = true;
		for (const auto &list : index) {
			int token = list.first;
			Node *new_child = new Node();
			node->children[token] = new_child;
			const vector<int> *list2;
			const auto it = index2.find(token);
			if (it != index2.end())
				list2 = &(it->second);
			else
				list2 = new vector<int>();
			BuildIndex(new_child, list.second, *list2, depth+1);
		}
	} else {
		node->hasSubtree = false;
		node->leafIds = ids1;
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

unordered_set<int> TreeIndex::getPrefixList(const Row &row) {
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

void TreeIndex::TreeSearch(Node *node, const Row &row, int depth, int calcPrefixListSizeOnly) {
	if (!node->hasSubtree) {
		debug_count_leaf_ += node->leafIds.size();
		if (calcPrefixListSizeOnly) {
			numEstimatedCandidates_ += node->leafIds.size();
		} else {
			for (int id : node->leafIds) {
				candidates_.insert(id);
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
        if (node->children.find( tokens[i] ) != node->children.end()) {
            this->TreeSearch(
                    node->children[ tokens[i] ],
                    row,
                    depth+1,
					calcPrefixListSizeOnly);
        }
    }
}

