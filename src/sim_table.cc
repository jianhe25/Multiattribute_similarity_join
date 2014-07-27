#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
using namespace std;

DEFINE_int32(exp_version, 0, "experiment version");
DEFINE_int32(index_version, 0, "index version, 0 means no index at all");

// Following variable used for debugging and statistics
int choosen_index_count[100];
double total_index_filter_time = 0.0;
double total_verify_time = 0.0;
FILE *fp = fopen("least_num_candidates.txt", "w");
FILE *fp_candidateSet = fopen("least_candidates_set.txt","w");

int num_total = 0, num_total1 = 0;

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
}

SimTable::~SimTable() {
}

int total_prefix_index_size = 0;
void SimTable::InitIndex(Table &table1, Table &table2, vector<Similarity> &sims) {
    double index_time = getTimeStamp();
	// Install index plugin, default is prefix_index
	if (FLAGS_index_version == 1) {
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
		tree_sims.push_back(sims[2]);
		tree_sims.push_back(sims[1]);
		tree_index->Build(table1, table2, tree_sims);
		treeIndexes_.push_back(tree_index);

		//tree_sims.clear();
		//tree_sims.push_back(sims[0]);
		//tree_sims.push_back(sims[1]);
		//tree_sims.push_back(sims[2]);
		//tree_index = new TreeIndex(UNORDERED_JOIN_TREE);
		//tree_index->Build(table1, table2, tree_sims);
		//treeIndexes_.push_back(tree_index);

		//tree_sims.clear();
		//tree_sims.push_back(sims[1]);
		//tree_sims.push_back(sims[2]);
		//tree_sims.push_back(sims[0]);
		//treeIndexes_[1]->Build(table1, table2, tree_sims);
		//
		//tree_sims.clear();
		//tree_sims.push_back(sims[2]);
		//tree_sims.push_back(sims[1]);
		//tree_sims.push_back(sims[0]);
		//treeIndexes_[2]->Build(table1, table2, tree_sims);

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
			//treeIndexes_[sims[i].colx]->Build(table1, table2, tree_sims);
		//}
	} else if (FLAGS_index_version == 3) {
		treeIndex_ = new TreeIndex(ORDERED_JOIN_TREE);
		treeIndex_->Build(table1, table2, sims);
	} else if (FLAGS_index_version == 4) {
		treeIndex_ = new TreeIndex(ORDERED_SEARCH_TREE);
		treeIndex_->Build(table1, table2, sims);
	}

	// Debug and statistics
    print_debug("Build Index time: %fs\n", getTimeStamp() - index_time);
	for (int c = 0; c < num_col_; ++c)
		choosen_index_count[c] = 0;
}
void SimTable::Init(Table &table1, Table &table2, const vector<Similarity> &sims) {
	double time = getTimeStamp();
	GenerateTokensOrGram(sims, table1, /*isColy=*/0);
	GenerateTokensOrGram(sims, table2, /*isColy=*/1);
	print_debug("GenerateTokensOrGram time: %.3fs\n", getTimeStamp() - time);

	GenerateContent(sims, table1, /*isColy=*/0);
	GenerateContent(sims, table2, /*isColy=*/1);
	time = getTimeStamp();
	print_debug("GenerateContent time: %.3fs\n", getTimeStamp() - time);

	time = getTimeStamp();
	initFilters();
    // TODO: currently no need for row matrix
    tablePtr_ = &table1;
	num_row_ = table1.size();
	num_col_ = table1[0].size();
	transpose(table1, &column_table1_);
	transpose(table2, &column_table2_);
    print_debug("Transpose time: %.3fs\n", getTimeStamp() - time);
}

vector<pair<RowID, RowID>> SimTable::Join(Table &table1, Table &table2, vector<Similarity> &sims) {
	Init(table1, table2, sims);
	InitIndex(table1, table2, sims);

	vector<pair<RowID, RowID>> simPairs;
	startJoinTime_ = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
		vector<RowID> results = JoinSearch(table2[i], sims);
		for (int id : results)
			simPairs.push_back(make_pair(id, i));
	}
	print_debug("num_total = %d num_total1 = %d ratio = %f\n", num_total, num_total1, num_total * 1.0 / num_total1);
    print_debug("Join time %.3fs\n", getTimeStamp() - startJoinTime_);
    print_debug("table size : %lu %lu\n", table1.size(), table2.size());

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
		cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!! No Similarity in ChooseBestIndexColumn" << endl;
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

	if (query_row[0].id % 1000 == 0) {
		print_debug("choose column %d\n", least_sim.colx);
	}
	choosen_index_count[ least_sim.colx ]++;
	return least_sim;
}

vector<RowID> SimTable::JoinSearch(Row &query_row, vector<Similarity> &sims) {
    vector<int> candidateIDs;
    double time = getTimeStamp();

	// Use old_set
	unordered_set<int> candidateSet;
	if (FLAGS_index_version == 0 || FLAGS_index_version == 1) {
		Similarity index_column = ChooseBestIndexColumn(query_row, sims);
		candidateIDs = std::move( indexes_[index_column.colx]->search(query_row[index_column.coly]) );
	} else if (FLAGS_index_version == 2) {
		//TreeIndex *best_tree_index = ChooseBestTreeIndex(query_row);
		TreeIndex *best_tree_index = treeIndexes_[0];
		candidateIDs = std::move( best_tree_index->getPrefixList(query_row) );
	} else if (FLAGS_index_version == 3 || FLAGS_index_version == 4) {
		candidateIDs = std::move( treeIndex_->getPrefixList(query_row) );
	}
	if (candidateIDs.size() == 0) {
		//print_debug("candidateIDs.size() = %lu\n",candidateIDs.size());
		//for (const Field grid : query_row) {
			//printf("%s ", grid.str.c_str());
		//}
		//puts("");
	}

	if (candidateIDs.empty()) {
		for (int id : candidateSet)
			candidateIDs.push_back(id);
	} else {
		sort(candidateIDs.begin(), candidateIDs.end());
		candidateIDs.erase(unique(candidateIDs.begin(), candidateIDs.end()), candidateIDs.end());
	}

	double index_filter_time = getTimeStamp() - time;
    total_index_filter_time += index_filter_time;
	int queryId = query_row[0].id;
	//print_debug("%d candidateIDs.size() = %d\n", queryId, candidateIDs.size());
	if (queryId % 1000 == 0) {
		print_debug("id: %d, get prefix_list time: %.5fs %.5fs candidates.size() = %d, Join time: %.5fs\n",
				queryId,
				index_filter_time,
				total_index_filter_time / queryId,
				int(candidateIDs.size()),
				getTimeStamp() - startJoinTime_);
	}
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
        cerr << "Error: NonExist exp_version, exp_version is in [" << 0 << ", " << 2 << "]" << endl;
        result = vector<RowID>(0);
	}
	double verify_time = getTimeStamp() - time;
    total_verify_time += verify_time;
	if (queryId % 1000 == 0)
		print_debug("Verify time: %.5fs %.5fs Join time: %.5fs\n", verify_time, total_verify_time / queryId, getTimeStamp() - startJoinTime_);
	return result;
}

Estimation SimTable::Estimate(const vector<Field*> &column,
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
		pass += filter->filter(*column[id], query, sim);
    }
	//print_debug("sampleIDs.size() = %d %d, pass = %d\n", sampleIDs.size(), ids.size(), pass);
	double cost = double(getTimeStamp() - startTime) / num_sample;
	double ratio = 1.0 - double(pass) / num_sample;
	return Estimation(ratio, cost, filter, &sim);
}

vector<RowID> SimTable::Search(Row &query_row, vector<Similarity> &sims) {
	print_debug("Search Not implemented yet\n");
}

void SimTable::InitTableForSearch(Table &table, const vector<Similarity> &sims) {
	print_debug("InitTableForSearch Not implemented yet\n");
}

