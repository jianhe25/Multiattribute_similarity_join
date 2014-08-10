#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
using namespace std;

DEFINE_int32(exp_version, 0, "experiment version");
DEFINE_int32(index_version, 0, "index version, 0 means no index at all");
DEFINE_int32(memory_limit, 100, "maximum memory, 100 means 100MB");

// Following variable used for debugging and statistics
int choosen_index_count[100];
double total_index_filter_time = 0.0;
double total_verify_time = 0.0;
//FILE *fp = fopen("least_num_candidates.txt", "w");
//FILE *fp_candidateSet = fopen("least_candidates_set.txt","w");

Estimation::Estimation(double _ratio, double _cost, Filter *_filter, Similarity *_sim) :
	ratio(_ratio), cost(_cost), filter(_filter), sim(_sim) {
}

bool Estimation::operator > (const Estimation &other) const {
	return !(*this < other);
}

bool Estimation::operator < (const Estimation &other) const {
	if (this->ratio == 0 && other.ratio == 0) {
		//print_debug("both 0\n");
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
	if (FLAGS_index_version == 1) {
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
		tree_index->BuildJoinTree(table1, table2, tree_sims);
		treeIndexes_.push_back(tree_index);
		//for (int i = 0; i < num_col_; ++i)
			//treeIndexes_.push_back(new TreeIndex(UNORDERED_JOIN_TREE));
		//for (int i = 0; i < int(sims.size()); ++i) {
			//vector<Similarity> tree_sims;
			//tree_sims.push_back(sims[i]);
			////if (i > 0)
			//for (int j = 0; j < int(sims.size()); ++j) {
				//if (j != i)
					//tree_sims.push_back(sims[j]);
			//}
				////else
				////tree_sims.push_back(sims[1]);
			//treeIndexes_[sims[i].colx]->BuildJoinTree(table1, table2, tree_sims);
		//}
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
	double startTime = getTimeStamp();
	GenerateTokensOrGram(sims, table1, /*isColy=*/0);
	GenerateTokensOrGram(sims, table2, /*isColy=*/1);
	print_debug("GenerateTokensOrGram time: %.3fs\n", getTimeStamp() - time);

	time = getTimeStamp();
	GenerateContent(sims, table1, /*isColy=*/0);
	GenerateContent(sims, table2, /*isColy=*/1);
	print_debug("GenerateContent time: %.3fs\n", getTimeStamp() - time);

	time = getTimeStamp();
	initFilters();
    // TODO: currently no need for row matrix
    tablePtr_ = &table1;
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
			simPairs.push_back(make_pair(id, i));
	}
    print_debug("Join time %.3fs\n", getTimeStamp() - startJoinTime_);
    print_debug("table size : %lu %lu\n", table1.size(), table2.size());

	print_debug("numCandidatePairs : %lld\n", numCandidatePairs_);
	print_debug("numRepeatCandidatePairs : %lld\n", numRepeatCandidatePairs_);
	print_debug("filter_time: %f, verify_time: %f\n", total_index_filter_time, total_verify_time);
	ExportTime("numCandidatePairs", numCandidatePairs_);
	ExportTime("Filter", total_index_filter_time);
	ExportTime("Verify", total_verify_time);
	for (int c = 0; c < num_col_; ++c)
		print_debug("choose column %d as index %d times\n", c, choosen_index_count[c]);
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
	//fprintf(fp,"id: %d, col %d, num_candidates: index %d TreeIndex = %d, delta = %d\n",query_row[0].id, least_sim.colx,
	//least_candidates_number1, least_candidates_number, least_candidates_number - least_candidates_number1);
	//if (query_row[0].id % 1000 == 0) {
		//for (const Similarity &sim : treeIndexes_[best_tree_index_id]->sims_) {
			//print_debug("%d\n", sim.colx);
		//}
		//print_debug("best_tree_index_id = %d\n", best_tree_index_id);
	//}
	choosen_index_count[best_tree_index_id]++;
	return treeIndexes_[best_tree_index_id];
}

Similarity SimTable::ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims) {
	if (sims.empty()) {
		print_debug("Error: No Similarity in ChooseBestIndexColumn");
		return Similarity();
	}
	int real_least = num_row_ + 1;
	//int least_candidates_number = real_least, least_candidates_number1 = real_least;
	Similarity least_sim;
	for (auto &sim : sims) {
		int num_estimated_candidates = indexes_[sim.colx]->calcPrefixListSize(query_row[sim.coly]);
		if (num_estimated_candidates < real_least) {
			real_least = num_estimated_candidates;
			least_sim = sim;
		}
		sim.num_estimated_candidates = num_estimated_candidates;
	}
	//fprintf(fp,"id: %d, col %d, num_candidates: index %d TreeIndex = %d, delta = %d\n",query_row[0].id, least_sim.colx,
	//least_candidates_number1, least_candidates_number, least_candidates_number - least_candidates_number1);

	//if (query_row[0].id % 1000 == 0) {
		//print_debug("choose column %d\n", least_sim.colx);
	//}
	choosen_index_count[ least_sim.colx ]++;
	return least_sim;
}

vector<RowID> SimTable::JoinSearch(Row &query_row, vector<Similarity> &sims) {
    vector<int> candidateIDs;
    double time = getTimeStamp();

	if (FLAGS_index_version == 0 || FLAGS_index_version == 1) {
		Similarity index_column = ChooseBestIndexColumn(query_row, sims);
		candidateIDs = std::move( indexes_[index_column.colx]->getPrefixList(query_row[index_column.coly]) );
	} else if (FLAGS_index_version == 2) {
		//TreeIndex *best_tree_index = ChooseBestTreeIndex(query_row);
		TreeIndex *best_tree_index = treeIndexes_[0];
		candidateIDs = std::move( best_tree_index->getPrefixList(query_row) );
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
	//print_debug("%d candidateIDs.size() = %d\n", queryId, candidateIDs.size());
	//if (queryId % 100000 == 0) {
		//print_debug("id: %d, get prefix_list time: %.5fs %.5fs candidates.size() = %d, Join time: %.5fs\n",
				//queryId,
				//index_filter_time,
				//total_index_filter_time / queryId,
				//int(candidateIDs.size()),
				//getTimeStamp() - startJoinTime_);
	//}
	time = getTimeStamp();

    vector<RowID> result;
	if (candidateIDs.size() == 0)
		return result;
	if (FLAGS_exp_version == 0) {
        result = Search0_NoEstimate(query_row, sims, candidateIDs);
    } else if (FLAGS_exp_version == 1) {
        result = Search1_Estimate(query_row, sims, candidateIDs);
    } else if (FLAGS_exp_version == 2) {
        result = Search2_TuneEstimate(query_row, sims, candidateIDs);
    } else {
        print_debug("Error: NonExist exp_version, exp_version is in [0,2]");
        result = vector<RowID>(0);
	}
	double verify_time = getTimeStamp() - time;
    total_verify_time += verify_time;
	//if (queryId % 100000 == 0)
	//print_debug("Verify time: %.5fs %.5fs Join time: %.5fs\n", verify_time, total_verify_time / queryId, getTimeStamp() - startJoinTime_);
	return result;
}

Estimation SimTable::Estimate(const Table &table,
							const Field &query,
							Similarity &sim,
							const vector<int> &ids,
							Filter *filter) {
	//int num_sample = 1;
	int num_sample = 5;
	int pass = 0;
	double startTime = getTimeStamp();
	for (int i = 0; i < num_sample; ++i) {
		int id = ids[rand() % ids.size()];
		pass += filter->filter(table[id][sim.colx], query, sim);
    }
	//print_debug("sampleIDs.size() = %d %d, pass = %d\n", sampleIDs.size(), ids.size(), pass);
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

void SimTable::CopyColumnY(vector<Similarity> *treeSims, const vector<Similarity> &querySims) {
	for (Similarity &sim1 : *treeSims) {
		for (const Similarity &sim2 : querySims) {
			if (sim1.colx == sim2.colx) {
				sim1.coly = sim2.coly;
				break;
			}
		}
	}
}

vector<RowID> SimTable::Search(Row &query_row, vector<Similarity> &sims) {
	for (const Similarity &sim : sims) {
		if (sim.distType == ED) {
			query_row[sim.coly].GenerateContent();
			query_row[sim.coly].GenerateGrams();
		} else {
			query_row[sim.coly].GenerateTokens();
		}
	}

	// TODO: use estimation later
	const int AVERAGE_VERIFY_COST = 5;

	vector<RowID> candidateIDs;
	vector<Similarity> union_sims;
	for (int i = 0; i < (int)treeIndexes_.size(); ++i) {
		if (contain(sims, treeIndexes_[i]->sims_) && !contain(union_sims, treeIndexes_[i]->sims_)) {
			CopyColumnY(&treeIndexes_[i]->sims_, sims);
			int list_size = treeIndexes_[i]->calcPrefixListSize(query_row);
			if (!candidateIDs.empty() &&
				list_size > int(candidateIDs.size()) * AVERAGE_VERIFY_COST) {
				return candidateIDs;
			}
			vector<int> list = treeIndexes_[i]->getPrefixList(query_row);
			if (candidateIDs.empty())
				candidateIDs = list;
			else
				candidateIDs = std::move( Intersect2Lists(candidateIDs, list) );
			union_sims = std::move( union2sims(union_sims, treeIndexes_[i]->sims_) );
			print_debug("candidateIDs.size() = %lu %lu\n",candidateIDs.size(), list.size());
		}
	}

	sort(candidateIDs.begin(), candidateIDs.end());
	candidateIDs.erase(unique(candidateIDs.begin(), candidateIDs.end()), candidateIDs.end());
	for (int id : candidateIDs)
		printf("%d ",id);
	puts("");

    vector<RowID> result;
	if (candidateIDs.size() == 0)
		return result;
	if (FLAGS_exp_version == 0) {
        result = Search0_NoEstimate(query_row, sims, candidateIDs);
    } else if (FLAGS_exp_version == 1) {
        result = Search1_Estimate(query_row, sims, candidateIDs);
    } else if (FLAGS_exp_version == 2) {
        result = Search2_TuneEstimate(query_row, sims, candidateIDs);
    } else {
        print_debug("Error: NonExist exp_version, exp_version is in [0, 2]");
        result = vector<RowID>(0);
	}

	return result;
}

/*
 * Sort tree by entropy, from big to small
 */
//bool compareByEntropy(TreeIndex *tree1, TreeIndex *tree2) {
//return tree1->treeEntropy_ > tree2->treeEntropy_;
//}

TreeIndex *SimTable::Concat2Trees(TreeIndex *tree1, TreeIndex *tree2) {
	vector<Similarity> temp_sims;
	if (tree1->treeEntropy_ > tree2->treeEntropy_) {
		temp_sims = tree1->sims_;
		temp_sims.insert(temp_sims.end(), tree2->sims_.begin(), tree2->sims_.end());
	} else {
		temp_sims = tree2->sims_;
		temp_sims.insert(temp_sims.end(), tree1->sims_.begin(), tree1->sims_.end());
	}
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

	GenerateContent(sims, table, /*isColy=*/0);
	time = getTimeStamp();
	print_debug("GenerateContent time: %.3fs\n", getTimeStamp() - time);

	time = getTimeStamp();
	initFilters();
    // TODO: add memory control later
    tablePtr_ = &table;
	num_row_ = table.size();
	num_col_ = table[0].size();

	print_debug("Start building search index...\n");
	/*
	 * Build one layer index
	 */
	for (int i = 0; i < int(sims.size()); ++i) {
		vector<Similarity> temp_sims;
		temp_sims.push_back(sims[i]);
		TreeIndex* tree_index = new TreeIndex(UNORDERED_SEARCH_TREE);
		tree_index->BuildSearchTree(table, temp_sims);
		treeIndexes_.push_back(tree_index);
		FLAGS_memory_limit -= treeIndex_->memory();
	}

	sort(treeIndexes_.begin(), treeIndexes_.end(), compareTreeByEntropyObject);
	reverse(treeIndexes_.begin(), treeIndexes_.end());

	int num_tree = treeIndexes_.size();
	for (int depth = 2; depth <= sims.size(); ++depth) {
		vector<pair<TreeIndex*, TreeIndex*>> tree_pairs;
		for (int i = 0; i < int(treeIndexes_.size()); ++i)
			for (int j = i+1; j < int(treeIndexes_.size()); ++j)
			if (treeIndexes_[i]->sims_.size() + treeIndexes_[j]->sims_.size() == depth &&
				!contain(treeIndexes_[i]->sims_, treeIndexes_[j]->sims_) &&
				!contain(treeIndexes_[j]->sims_, treeIndexes_[i]->sims_)) {
				tree_pairs.push_back(make_pair(treeIndexes_[i], treeIndexes_[j]));
			}
		sort(tree_pairs.begin(), tree_pairs.end(), compareByPairEntropy);
		for (const auto tree_pair : tree_pairs) {
			TreeIndex *newtree = Concat2Trees(tree_pair.first, tree_pair.second);
			treeIndexes_.push_back(newtree);
			FLAGS_memory_limit -= newtree->memory();
			if (++num_tree > 10) break;
			if (FLAGS_memory_limit < 0)
				break;
		}
		if (num_tree > 10) break;
		if (FLAGS_memory_limit < 0)
			break;
	}
	sort(treeIndexes_.begin(), treeIndexes_.end(), compareTreeByEntropyObject);
	for (const auto &tree : treeIndexes_) {
		print_debug("tree.depth %d entropy : %f\n", tree->sims_.size(), tree->treeEntropy_);
	}
	//sort(treeIndexes_.begin(), treeIndexes_.end(), CompareTreeByEntropy);
	print_debug("End building search index, takes %fs\n", getTimeStamp() - time);
}

