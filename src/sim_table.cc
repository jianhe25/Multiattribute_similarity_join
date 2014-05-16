#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
using namespace std;

DEFINE_int32(exp_version, 0, "experiment version");

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

void SimTable::InitIndex(Table &table1, Table &table2, vector<Similarity> &sims) {
    double index_time = getTimeStamp();
	indexes_.clear();
	// Install index plugin, default is prefix_index
	//if (FLAGS_index_version == 0 ||  FLAGS_index_version == 1) {
		SimIndexFactory::InstallIndex();
		for (int c = 0; c < num_col_; ++c)
			indexes_.push_back(SimIndexFactory::GetIndex()->GetInstance());

		for (auto &sim : sims) {
			indexes_[sim.colx]->build(column_table1_[sim.colx], column_table2_[sim.coly], &sim);
		}
		//}
	// This is PREFIX_TREE_INDEX
	//else if (FLAGS_exp_version == 2) {
		for (int i = 0; i < num_col_; ++i)
			treeIndexes_.push_back(new TreeIndex());
		for (int i = 0; i < int(sims.size()); ++i) {
			vector<Similarity> tree_sims;
			tree_sims.push_back(sims[i]);
			if (i > 0)
				tree_sims.push_back(sims[0]);
			else
				tree_sims.push_back(sims[1]);
			treeIndexes_[sims[i].colx]->Build(table1, table2, tree_sims);
			const auto &sim = sims[i];
		}
		//}

	// Debug and statistics
    print_debug("Build Index time: %.3fs\n", getTimeStamp() - index_time);
	for (int c = 0; c < num_col_; ++c)
		choosen_index_count[c] = 0;
}
void SimTable::Init(Table &table1, Table &table2) {
    double tranpose_time = getTimeStamp();
	initFilters();
    // TODO: currently no need for row matrix
    tablePtr_ = &table1;
	num_row_ = table1.size();
	num_col_ = table1[0].size();
	transpose(table1, &column_table1_);
	transpose(table2, &column_table2_);
    print_debug("Transpose time: %.3fs\n", getTimeStamp() - tranpose_time);
}

vector<pair<RowID, RowID>> SimTable::Join(Table &table1, Table &table2, vector<Similarity> &sims) {
	Init(table1, table2);
	InitIndex(table1, table2, sims);
	vector<pair<RowID, RowID>> simPairs;
	startJoinTime_ = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
		vector<RowID> results = Search(table2[i], sims);
		for (int id : results)
			simPairs.push_back(make_pair(id, i));
	}
    print_debug("Join time %.3fs\n", getTimeStamp() - startJoinTime_);
    print_debug("table size : %lu %lu\n", table1.size(), table2.size());

	for (int c = 0; c < num_col_; ++c)
		print_debug("choose column %d as index %d times\n", c, choosen_index_count[c]);
	return simPairs;
}

bool compareSimSize(const Similarity &a, const Similarity &b) {
	return a.num_estimated_candidates < b.num_estimated_candidates;
}
FILE *fp = fopen("num_candidates.txt","w");
Similarity SimTable::ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims) {
	if (sims.empty()) {
		cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!! No Similarity in ChooseBestIndexColumn" << endl;
		return Similarity();
	}
	int least_candidates_number = num_row_ + 1;
	Similarity least_sim;
	for (auto &sim : sims) {
		int num_estimated_candidates = 0;
		//if (FLAGS_index_version == 0 || FLAGS_index_version == 1) {
			int num_estimated_candidates1 = indexes_[sim.colx]->calcPrefixListSize(query_row[sim.coly]);
			//} else {
			num_estimated_candidates = treeIndexes_[sim.colx]->calcPrefixListSize(query_row);
			//}
		fprintf(fp,"id: %d, col %d, num_candidates: index %d TreeIndex = %d, delta = %d\n",query_row[0].id, sim.colx,
				num_estimated_candidates1, num_estimated_candidates, num_estimated_candidates - num_estimated_candidates1);

		if (num_estimated_candidates < least_candidates_number) {
			least_candidates_number = num_estimated_candidates;
			least_sim = sim;
		}
		sim.num_estimated_candidates = num_estimated_candidates;
	}
	if (query_row[0].id % 1000 == 0) {
		print_debug("choose column %d\n", least_sim.colx);
	}
	choosen_index_count[ least_sim.colx ]++;
	return least_sim;
}

unordered_set<int> intersect2Sets(unordered_set<int> setA, unordered_set<int> setB) {
    unordered_set<int> inter_set;
    if (setA.size() < setB.size()) {
        for (int id : setA)
            if (setB.find(id) != setB.end())
                inter_set.insert(id);
    } else {
        for (int id : setB)
            if (setA.find(id) != setA.end())
                inter_set.insert(id);
    }
    return inter_set;
}

vector<RowID> SimTable::Search(Row &query_row, vector<Similarity> &sims) {
    vector<int> candidateIDs;
    double time = getTimeStamp();

#ifdef INTERSECT_PREFIX_LIST
	for (auto &sim : sims)
		sim.num_estimated_candidates = indexes_[sim.colx].calcPrefixListSize(query_row[sim.coly]);
	sort(sims.begin(), sims.end(), compareSimSize);
	unordered_set<int> new_candidates;
	for (int i = 0; i < int(2); ++i) {
		unordered_set<int> candidates = std::move(indexes_[sims[i].colx].getPrefixList(query_row[sims[i].coly]));
		if (i == 0)
			new_candidates = candidates;
		else
			new_candidates = intersect2Sets(new_candidates, candidates);
	}
	for (int id : new_candidates)
		candidateIDs.push_back(id);
#else
	Similarity index_column = ChooseBestIndexColumn(query_row, sims);
	// Use old_set
	unordered_set<int> candidateSet;
	if (FLAGS_index_version == 0 || FLAGS_index_version == 1)
		candidateSet = std::move( indexes_[index_column.colx]->getPrefixList(query_row[index_column.coly]) );
	else
		candidateSet = std::move( treeIndexes_[index_column.colx]->getPrefixList(query_row) );
	for (int id : candidateSet)
		candidateIDs.push_back(id);
#endif

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

	/*
	 * Move index_column to first Similarity
	 * This operations is very slow: add 10s in 25s
	 */
	//vector<Similarity> new_sims;
	//new_sims.push_back(*index_column);
	//for (const Similarity &sim : sims) {
		//if (sim.coly != index_column->coly)
			//new_sims.push_back(sim);
	//}
	//sims = new_sims;

    vector<RowID> result;
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
