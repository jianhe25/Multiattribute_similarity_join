#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <set>
using namespace std;

DEFINE_int32(verify_exp_version, 0, "verify_experiment version");
DEFINE_int32(index_version, 0, "index version, 0 means no index at all");
DEFINE_double(memory_control, 1, "allocate memory_control * single_buffer_size for multi-trees");
DEFINE_string(search_exp, "dynamic_search", "3 different exps: dynamic_search, verify_directly, intersect_only");
DEFINE_double(total_filter_time, 0, "time for filtering");
DEFINE_string(baseline_exp, "prefixjoin", "edjoin+ppjoin");
DEFINE_int32(estimate_filter_period, 10000, "How long we verify filters");

// Following variable used for debugging and statistics
int choosen_index_count[100];
double total_index_filter_time = 0.0;
double total_verify_time = 0.0;

Estimation::Estimation(double _ratio, double _cost, Filter *_filter, Similarity *_sim) :
	ratio(_ratio), cost(_cost), filter(_filter), sim(_sim) {
}

bool Estimation::operator > (const Estimation &other) const {
	return !(*this < other);
}

bool Estimation::operator < (const Estimation &other) const {
	if (this->ratio == 0 && other.ratio == 0) {
		return this->cost < other.cost;
	}
	return this->ratio * other.cost > this->cost * other.ratio;
	// aka. ratio / cost > other.ratio / other.cost
}

SimTable::SimTable() {
	numCandidatePairs_ = 0;
}

SimTable::~SimTable() {
}

int total_prefix_index_size = 0;
void SimTable::InitJoinIndex(Table &table1, Table &table2, vector<Similarity> &sims) {
    double index_time = getTimeStamp();
	// Install index plugin, default is prefix_index
	if (FLAGS_index_version == 1 || FLAGS_index_version == 5 || FLAGS_index_version == 6) {
		transpose(table1, &column_table1_);
		transpose(table2, &column_table2_);
		for (int c = 0; c < num_col_; ++c)
			indexes_.push_back(new PrefixIndex());
		for (auto &sim : sims) {
			indexes_[sim.colx]->build(column_table1_[sim.colx], column_table2_[sim.coly], &sim);
			total_prefix_index_size += indexes_[sim.colx]->indexSize_;
		}
		print_debug("total_prefix_index_size = %d\n", total_prefix_index_size);
	}
	//This is PREFIX_TREE_INDEX
	else if (FLAGS_index_version == 2) {
		TreeIndex* tree_index = new TreeIndex(UNORDERED_JOIN_TREE);
		vector<Similarity> tree_sims;
		tree_sims.push_back(sims[0]);
		tree_sims.push_back(sims[1]);
		tree_sims.push_back(sims[2]);
		tree_sims.push_back(sims[3]);
		//std::random_shuffle(tree_sims.begin(), tree_sims.end());
		for (auto sim : tree_sims)
			print_debug("sim = %d\n", sim.colx);
		tree_index->BuildJoinTree(table1, table2, tree_sims);
		treeIndexes_.push_back(tree_index);
	} else if (FLAGS_index_version == 3) {
		treeIndex_ = new TreeIndex(ORDERED_JOIN_TREE);
		treeIndex_->BuildJoinTree(table1, table2, sims);
	} else if (FLAGS_index_version == 4) {
		treeIndex_ = new TreeIndex(OPTIMAL_JOIN_TREE);
		treeIndex_->BuildJoinTree(table1, table2, sims);
	}
	// Debug and statistics
    print_debug("Build Index time: %fs\n", getTimeStamp() - index_time);
	ExportTime("Indexing", getTimeStamp() - index_time);
	for (int c = 0; c < num_col_; ++c)
		choosen_index_count[c] = 0;
}
void SimTable::InitJoin(Table &table1, Table &table2, const vector<Similarity> &sims) {
	double time = getTimeStamp();
	GenerateTokensOrGram(sims, table1, /*isColy=*/0);
	GenerateTokensOrGram(sims, table2, /*isColy=*/1);
	print_debug("GenerateTokensOrGram time: %.3fs\n", getTimeStamp() - time);
	time = getTimeStamp();
	initFilters();
    // TODO: currently no need for row matrix
    tablePtr_ = &table1;
	tablePtr2_ = &table2;
	num_row_ = table1.size();
	num_col_ = table1[0].size();
}

vector<pair<RowID, RowID>> SimTable::Join(Table &table1, Table &table2, vector<Similarity> &sims) {
	InitJoin(table1, table2, sims);
	InitJoinIndex(table1, table2, sims);
	vector<pair<RowID, RowID>> simPairs;
	startJoinTime_ = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
		vector<RowID> results = JoinSearch(table2[i], sims);
		for (int id : results)
			simPairs.emplace_back(id, i);
	}

	for (int c = 0; c < num_col_; ++c)
		print_debug("choose column %d as index %d times\n", c, choosen_index_count[c]);
	ExportTime("numCandidatePairs", numCandidatePairs_);
	ExportTime("Filter", total_index_filter_time);
	ExportTime("Verify", total_verify_time);
	return simPairs;
}

bool compareSimSize(const Similarity &a, const Similarity &b) {
	return a.num_estimated_candidates < b.num_estimated_candidates;
}

TreeIndex* SimTable::ChooseBestTreeIndex(Row &query_row) {
	int real_least = num_row_ + 1;
	int best_tree_index_id = -1;
	for (int i = 0; i < (int)treeIndexes_.size(); ++i) {
		int num_estimated_candidates = 0;
		num_estimated_candidates = treeIndexes_[i]->calcPrefixListSize(query_row);
		if (num_estimated_candidates < real_least) {
			real_least = num_estimated_candidates;
			best_tree_index_id = i;
		}
	}
	choosen_index_count[best_tree_index_id]++;
	return treeIndexes_[best_tree_index_id];
}

Similarity SimTable::ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims) {
	if (sims.empty()) {
		print_debug("Error: No Similarity in ChooseBestIndexColumn");
		return Similarity();
	}
	int real_least = tablePtr_->size() + 1;
	Similarity least_sim;
	for (auto &sim : sims) {
		int num_estimated_candidates = indexes_[sim.colx]->calcPrefixListSize(query_row[sim.coly], sim);
		if (query_row[0].id % 1000 == 0)
			print_debug("sim : %d num_estimated_candidates : %d\n", sim.colx, num_estimated_candidates);
		if (num_estimated_candidates < real_least) {
			real_least = num_estimated_candidates;
			least_sim = sim;
		}
		sim.num_estimated_candidates = num_estimated_candidates;
	}
	choosen_index_count[ least_sim.colx ]++;
	return least_sim;
}

bool compareSimByNumCandidates(const Similarity& a, const Similarity& b) {
	return a.num_estimated_candidates < b.num_estimated_candidates;
}
vector<RowID> SimTable::JoinSearch(Row &query_row, vector<Similarity> &sims) {
    vector<int> candidateIDs;
    double time = getTimeStamp();

	if (FLAGS_index_version == 0 || FLAGS_index_version == 1) { // use single best attribute only
		Similarity index_column = ChooseBestIndexColumn(query_row, sims);
		candidateIDs = indexes_[index_column.colx]->getPrefixList(query_row[index_column.coly], index_column);
	} else if (FLAGS_index_version == 2) {
		candidateIDs = std::move( treeIndexes_[0]->getPrefixList(query_row) );
	}
	else if (FLAGS_index_version == 5 || FLAGS_index_version == 6) { // intersect all attributes
		for (auto &sim : sims) {
			int num_estimated_candidates = indexes_[sim.colx]->calcPrefixListSize(query_row[sim.coly], sim);
			sim.num_estimated_candidates = num_estimated_candidates;
		}
		sort(sims.begin(), sims.end(), compareSimByNumCandidates);
		int limit = sims.size(); // for FLAGS_index_version == 5
		if (FLAGS_index_version == 6)
			limit = min(2, int(sims.size()));
		for (int i = 0; i < limit; ++i) {
			vector<int> temp_list = indexes_[sims[i].colx]->getPrefixList(query_row[sims[i].coly], sims[i]);
			if (i == 0)
				candidateIDs = temp_list;
			else
				candidateIDs = Intersect2Lists(candidateIDs, temp_list);
			if (candidateIDs.empty()  || candidateIDs.size() < 10 )
				break;
		}
	} else if (FLAGS_index_version == 3 || FLAGS_index_version == 4) {
		candidateIDs = std::move( treeIndex_->getPrefixList(query_row) );
	}

	numRepeatCandidatePairs_ += candidateIDs.size();
	sort(candidateIDs.begin(), candidateIDs.end());
	candidateIDs.erase(unique(candidateIDs.begin(), candidateIDs.end()), candidateIDs.end());

	numCandidatePairs_ += candidateIDs.size();
	double index_filter_time = getTimeStamp() - time;
    total_index_filter_time += index_filter_time;
	int queryId = query_row[0].id;
	time = getTimeStamp();

    vector<RowID> result;
	if (candidateIDs.size() == 0) {
		result = vector<RowID>(0);
	} else if (FLAGS_verify_exp_version == 0) {
        result = Search0_NoEstimate(query_row, sims, candidateIDs);
    } else if (FLAGS_verify_exp_version == 1) {
        result = Search1_Estimate(query_row, sims, candidateIDs);
    } else if (FLAGS_verify_exp_version == 2) {
        result = Search2_TuneEstimate(query_row, sims, candidateIDs);
    } else {
        print_debug("Error: NonExist verify_exp_version, verify_exp_version is in [0,2]");
        result = vector<RowID>(0);
	}
	double verify_time = getTimeStamp() - time;
    total_verify_time += verify_time;
	if (queryId % 1000 == 0) {
		double enlarge_factor = (double)(tablePtr2_->size()) / queryId;
		print_debug("%d %lld %.3f %.3f %.3f %f\n", queryId,
				(long long)enlarge_factor * numCandidatePairs_,
				enlarge_factor * total_index_filter_time,
				enlarge_factor * total_verify_time,
				enlarge_factor * (getTimeStamp() - startJoinTime_),
				sims[0].dist);
	}
	return result;
}

Estimation SimTable::Estimate(const Table &table,
							const Field &query,
							Similarity &sim,
							const vector<int> &ids,
							Filter *filter) {
	int num_sample = 5;
	int pass = 0;
	double startTime = getTimeStamp();
	for (int i = 0; i < num_sample; ++i) {
		int id = ids[rand() % ids.size()];
		pass += filter->filter(table[id][sim.colx], query, sim);
    }
	double cost = double(getTimeStamp() - startTime) / num_sample;
	double ratio = 1.0 - double(pass) / num_sample;
	return Estimation(ratio, cost, filter, &sim);
}

vector<int> SimTable::Intersect2Lists(vector<int> &a, vector<int> &b) {
	vector<int> intersection;
	sort(a.begin(), a.end());
	sort(b.begin(), b.end());
	const vector<int> &small_list = a.size() < b.size()? a : b;
	const vector<int> &large_list = a.size() > b.size()? a : b;
	int now = 0;
	for (int i = 0; i < int(small_list.size()); ++i) {
		if (now < int(large_list.size()) && large_list[now] < small_list[i])
			++now;
		if (large_list[now] == small_list[i])
			intersection.push_back(large_list[now]);
	}
	return intersection;
}

bool SimTable::contain(const vector<Similarity> &treeSims, const Similarity &sim) {
	for (const Similarity &treesim : treeSims)
		if (treesim.colx == sim.colx) {
			return true;
		}
	return false;
}
bool SimTable::contain(const vector<Similarity> &querySims, const vector<Similarity> &treeSims) {
	for (const Similarity &sim1 : treeSims) {
		bool found = false;
		for (const Similarity &sim2 : querySims) {
			if (sim1.colx == sim2.colx) {
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

vector<Similarity> union2sims(const vector<Similarity> &sims1, const vector<Similarity> &sims2) {
	vector<Similarity> union_sims = sims1;
	for (const Similarity &sim2 : sims2) {
		bool found = false;
		for (const Similarity &union_sim : union_sims)
			if (sim2.colx == union_sim.colx) {
				found = true;
				break;
			}
		if (!found)
			union_sims.push_back(sim2);
	}
	return union_sims;
}

void SimTable::CopySimToTreeSim(vector<Similarity> *treeSims, const vector<Similarity> &querySims) {
	for (Similarity &sim1 : *treeSims) {
		for (const Similarity &sim2 : querySims) {
			if (sim1.colx == sim2.colx) {
				sim1.coly = sim2.coly;
				sim1.dist = sim2.dist;
				break;
			}
		}
	}
}

vector<RowID> SimTable::Search(Row &query_row, vector<Similarity> &sims) {
	vector<RowID> candidateIDs;
	double time = getTimeStamp();
	if (FLAGS_index_version == 1 || FLAGS_index_version == 5 || FLAGS_index_version == 6) {
        JoinSearch(query_row, sims);
    }
	if (FLAGS_index_version == 3) {
		int estimate_verify_cost = 0;
		for (const Similarity &sim : sims) {
			estimate_verify_cost += query_row[sim.coly].tokens.size();
		}
		vector<Similarity> union_sims;
		int round = 0;
		while (true) {
			TreeIndex *best_tree = NULL;
			int min_list_size = (int) tablePtr_->size();
			for (int i = 0; i < (int)treeIndexes_.size(); ++i) {
				if (contain(sims, treeIndexes_[i]->sims_) && !contain(union_sims, treeIndexes_[i]->sims_)) {
					CopySimToTreeSim(&treeIndexes_[i]->sims_, sims);
					int list_size = treeIndexes_[i]->calcPrefixListSize(query_row);
					if (list_size < min_list_size) {
						min_list_size = list_size;
						best_tree = treeIndexes_[i];
					}
				}
			}
			if (best_tree == NULL)
				break;
			if (round > 0) {
				if (FLAGS_search_exp == "dynamic_search") {
					long long cost1 = candidateIDs.size() * estimate_verify_cost;
					long long cost2  = min_list_size  + candidateIDs.size() + min(min_list_size, int(candidateIDs.size())) * 0.0001 * estimate_verify_cost;
					if (cost2 > cost1)
						break;
				} else if (FLAGS_search_exp == "verify_directly") {
					break;
				} else if (FLAGS_search_exp == "intersect_only") {
				} else {
					print_debug("Error: NonExist search exp: %s", FLAGS_search_exp.c_str());
				}
			}
			vector<int> list = std::move(best_tree->getPrefixList(query_row));
			if (round == 0)
				candidateIDs = list;
			else
				candidateIDs = Intersect2Lists(candidateIDs, list);
			if (candidateIDs.empty())
				break;
			union_sims = std::move( union2sims(union_sims, best_tree->sims_) );
			++round;
		}
	}

	numRepeatCandidatePairs_ += candidateIDs.size();
	sort(candidateIDs.begin(), candidateIDs.end());
	candidateIDs.erase(unique(candidateIDs.begin(), candidateIDs.end()), candidateIDs.end());
	numCandidatePairs_ += candidateIDs.size();
	double index_filter_time = getTimeStamp() - time;
    total_index_filter_time += index_filter_time;
	int queryId = query_row[0].id;
	time = getTimeStamp();

	for (unsigned i = 0; i < query_row.size(); ++i) query_row[i].id = 2;
    vector<RowID> result;
	if (candidateIDs.size() == 0)
		return result;
	if (FLAGS_verify_exp_version == 0) {
        result = Search0_NoEstimate(query_row, sims, candidateIDs);
    } else if (FLAGS_verify_exp_version == 1) {
        result = Search1_Estimate(query_row, sims, candidateIDs);
    } else if (FLAGS_verify_exp_version == 2) {
        result = Search2_TuneEstimate(query_row, sims, candidateIDs);
    } else {
        print_debug("Error: NonExist exp_version, exp_version is in [0, 2]");
        result = vector<RowID>(0);
	}
	total_verify_time += getTimeStamp() - time;

	if (queryId % 1000 == 0) {
		double enlarge_factor = (double)100000 / queryId;
		print_debug("%d %lld %.3f %.3f %.3f\n", queryId,
				(long long)enlarge_factor * numCandidatePairs_,
				enlarge_factor * total_index_filter_time,
				enlarge_factor * total_verify_time,
				enlarge_factor * (getTimeStamp() - startJoinTime_));
	}
	return result;
}

TreeIndex *SimTable::ConcatTreeSim(const TreeIndex *tree1, const Similarity &sim) {
	vector<Similarity> temp_sims = tree1->sims_;
	temp_sims.push_back(sim);
	TreeIndex* newtree = new TreeIndex(UNORDERED_SEARCH_TREE);
	newtree->BuildSearchTree(*tablePtr_, temp_sims);
	return newtree;
}

bool compareByPairEntropy(pair<TreeIndex*, TreeIndex*> p1, pair<TreeIndex*, TreeIndex*> p2) {
	return p1.first->treeEntropy_ * p1.second->treeEntropy_ < p2.first->treeEntropy_ * p2.second->treeEntropy_;
}

void SimTable::InitSearch(Table &table, const vector<Similarity> &sims) {
	double time = getTimeStamp();
	GenerateTokensOrGram(sims, table, /*isColy=*/0);
	print_debug("GenerateTokensOrGram time: %.3fs\n", getTimeStamp() - time);

	initFilters();
    tablePtr_ = &table;
	time = getTimeStamp();
	num_row_ = table.size();
	num_col_ = table[0].size();
	if (FLAGS_index_version == 1 || FLAGS_index_version == 5 || FLAGS_index_version == 6) {
		auto table2 = table;
		transpose(table, &column_table1_);
		transpose(table2, &column_table2_);
		for (int c = 0; c < num_col_; ++c)
			indexes_.push_back(new PrefixIndex());
		auto sims_copy = sims;
		for (auto &sim : sims_copy) {
			print_debug("sim.distType = %d\n", sim.distType);
			indexes_[sim.colx]->build(column_table1_[sim.colx], column_table2_[sim.coly], &sim);
		}
		print_debug("Init baseline search index %f\n", getTimeStamp() - time);
		startJoinTime_ = getTimeStamp();
		return;
	}
	print_debug("Start building search index...\n");
	long long single_buffer_size = 0;
	set<int> sim_bit_set;
	/*
	 * Build one layer index
	 */
	for (int i = 0; i < int(sims.size()); ++i) {
		vector<Similarity> temp_sims;
		temp_sims.push_back(sims[i]);
		TreeIndex* tree_index = new TreeIndex(UNORDERED_SEARCH_TREE);
		tree_index->BuildSearchTree(table, temp_sims);
		treeIndexes_.push_back(tree_index);
		single_buffer_size += tree_index->size();
		int sim_bit = 1 << sims[i].colx;
		sim_bit_set.insert(sim_bit);
	}
	long long multi_buffer_size = single_buffer_size * (FLAGS_memory_control - 1);

	for (unsigned i = 0; i < treeIndexes_.size(); ++i) {
		print_debug("tree with sim %d, size : %f\n", treeIndexes_[i]->sims_[0].colx, double(treeIndexes_[i]->size()) / single_buffer_size);
	}

	TreeIndex::SingleBufferSize = single_buffer_size;
	/*
	 * Select (tree,sim) with max benefit / cost among all combinations, use heap for dynamic update
	 */
	priority_queue<TreeSimPair> treeSimPairHeap;
	for (unsigned i = 0; i < treeIndexes_.size(); ++i) {
		for (unsigned j = 0; j < sims.size(); ++j) {
			if (!contain(treeIndexes_[i]->sims_, sims[j])) {
				pair<double,double> benefit_and_cost = treeIndexes_[i]->EstimateBenifitAndCost(sims[j]);
				if (benefit_and_cost.first > 0.0 && benefit_and_cost.second > 0.0)
					treeSimPairHeap.push(TreeSimPair(i, j, benefit_and_cost.first, benefit_and_cost.second));
			}
		}
	}

	while (multi_buffer_size > 0 && !treeSimPairHeap.empty()) {
		TreeSimPair treesim = treeSimPairHeap.top();
		treeSimPairHeap.pop();
		const TreeIndex *tree = treeIndexes_[ treesim.treeid ];
		const Similarity &sim = sims[treesim.simid];

		int sim_bit = 1 << sim.colx;
		for (const auto &sim : tree->sims_)
			sim_bit |= 1 << sim.colx;
		if (sim_bit_set.find(sim_bit) != sim_bit_set.end()) {
			continue;
		}

		TreeIndex *newtree = ConcatTreeSim(tree, sim);
		treeIndexes_.push_back(newtree);
		multi_buffer_size -= newtree->size();

		sim_bit = 0;
		for (const auto &sim : newtree->sims_)
			sim_bit |= 1 << sim.colx;
		sim_bit_set.insert(sim_bit);
		print_debug("select %d %d %f %f %f\n", treesim.treeid, treesim.simid, treesim.benefit, treesim.cost / single_buffer_size, double(newtree->size()) / single_buffer_size);
		if (multi_buffer_size <= 0) {
			break;
		}
		int newtree_id = treeIndexes_.size() - 1;
		for (unsigned j = 0; j < sims.size(); ++j) {
			if (!contain(newtree->sims_, sims[j])) {
				pair<double,double> benefit_and_cost = newtree->EstimateBenifitAndCost(sims[j]);
				if (benefit_and_cost.first > 0.0 && benefit_and_cost.second > 0.0)
					treeSimPairHeap.push(TreeSimPair(newtree_id, j, benefit_and_cost.first, benefit_and_cost.second));
			}
		}
	}

	sort(treeIndexes_.begin(), treeIndexes_.end(), compareTreeByEntropyObject);
	for (const auto &tree : treeIndexes_) {
		print_debug("tree.depth %d  entropy : %f, sims: ", int(tree->sims_.size()), tree->treeEntropy_);
		for (const auto sim : tree->sims_)
			printf("%d ", sim.colx);
		puts("");
	}
	print_debug("End building search index, takes %fs\n", getTimeStamp() - time);
	startJoinTime_ = getTimeStamp();
}

