#include "tree_index.h"
#include <cassert>
#include <algorithm>

#define PREFIX_BUCKET_SIZE 100

int Node::g_node_id = 0;
Node::Node() {
	id = ++g_node_id;
	hasSubtree = false;
	leafIds.clear();
}

TreeIndex::TreeIndex() {
}

TreeIndex::~TreeIndex() {
    for (int i = 0; i < 100; ++i)
        token_counter_[i].clear();
}

void TreeIndex::CalcTF() {
    for (const auto &row : *tablePtr1_) {
        for (const auto &sim : sims_)
            for (int token : row[sim.colx].tokens)
                token_counter_[sim.colx][token]++;
	}

    for (auto &row : *tablePtr1_) {
        for (const auto &sim : sims_) {
            std::sort(row[sim.colx].tokens.begin(), row[sim.colx].tokens.end(), CompareTokenByTF(token_counter_[sim.colx]));
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
	for (unsigned id = 0; id < table1.size(); ++id)
		row_ids1[id] = id;

	root_ = new Node();

    this->CalcTF();

	//print_debug("col %d %d sim.size = %lu\n",this->sims_[0].colx, this->sims_[1].colx, this->sims_.size());

	//freopen("result", "w", stdout);
	BuildIndex(root_,
               row_ids1,
               /*depth=*/0,
               /*hasSubtree=*/true);

    // Sort tokens again by their value instead of TF, so we can verify jaccard later
    for (auto &row : *tablePtr1_) {
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
                           int depth,
                           bool hasSubtree) {

	node->list = ids1; // TODO: for debug only
    node->hasSubtree = hasSubtree;
    node->leafIds = ids1;
	if (!hasSubtree) {
        //node->leafIds = ids1;
		//cout << node->id << " #leaf " << node->leafIds.size() << endl;
		return;
	}

    unordered_map<int, vector<int>> index;
    const Similarity &sim = sims_[depth];
    for (int id : ids1) {
		const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		//print_debug("depth = %d tokens.size() = %d\n", depth, tokens.size());
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		//print_debug("prefixlength = %d\n", prefixlength);
		for (int i = 0; i < prefixlength; ++i) {
            auto &list = index[ tokens[i] ];
            if (list.empty() || list.back() != id)
                list.push_back(id);
        }
	}

    /*
     * Build recursive prefix index
     * 1) Sort list by list_length
     * 2) Select long list to split, and split by other attributes
     *      2.1) Select list that has less repeated tokens.
     *      2.2) number of new token after add list >= 0.7 * sum(list.size())
     *      2.3) list should be long enough to split, otherwise is meaningless
     */
    int total = 0;
    for (auto list : index) {
        total += list.second.size();
    }
    vector<pair<int, vector<int>>> sorted_lists;
    for (auto list : index) {
        sorted_lists.push_back(make_pair(list.first, list.second));
    }
    // sort list by list size, from large to small
    std::sort(sorted_lists.begin(), sorted_lists.end(), CompareListPair);

    int num_large_list = 0;
    int list_place = 0;
    unordered_map<int, int> tokenMap;
    for (const auto &list : sorted_lists) {
        ++list_place;
        int num_new_id = 0;
        for (int id : list.second) {
            if (tokenMap.find(id) == tokenMap.end()) {
                num_new_id++;
            }
        }
        int token = list.first;
        Node *new_child = new Node();
        node->children[token] = new_child;
        if (depth < 1 &&
                num_new_id > 0.5 * int(list.second.size()) && list_place < int(sorted_lists.size()) / 3) {
            BuildIndex(new_child, list.second, depth+1, /*hasSubtree=*/true);
            for (int id : list.second)
                tokenMap[id]++;
            num_large_list += list.second.size();
        } else {
            BuildIndex(new_child, list.second, depth+1, /*hasSubtree=*/false);
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

	const auto &list = node->list;

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

