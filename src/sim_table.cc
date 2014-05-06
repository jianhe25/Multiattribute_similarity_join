#include "sim_table.h"
#include "common.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
using namespace std;

DEFINE_int32(exp_version, 0, "experiment version");

Estimation::Estimation(double _ratio, double _cost, Filter *_filter, Similarity *_sim) :
	ratio(_ratio), cost(_cost), filter(_filter), sim(_sim) {
}

bool Estimation::operator > (const Estimation &other) const {
	return this->ratio * other.cost < this->cost * other.ratio;
	// aka. ratio / cost < other.ratio / other.cost
}

bool Estimation::operator < (const Estimation &other) const {
	return !(*this > other);
}

SimTable::SimTable() {
}

SimTable::~SimTable() {
}

void SimTable::InitIndex(vector<Similarity> &sims) {
    double index_time = getTimeStamp();
	// Install index plugin, default is prefix_index
	indexes.clear();
	SimIndexFactory::InstallIndex();
	for (int c = 0; c < num_col_; ++c)
		indexes.push_back(SimIndexFactory::GetIndex()->GetInstance());

	for (auto &sim : sims) {
		indexes[sim.colx]->build(column_table1_[sim.colx], column_table2_[sim.coly], &sim);
    }
    print_debug("Build Index time: %.3fs\n", getTimeStamp() - index_time);
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
	InitIndex(sims);
	vector<pair<RowID, RowID>> simPairs;
	startJoinTime = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
		vector<RowID> results = Search(table2[i], sims);
		for (int id : results)
			simPairs.push_back(make_pair(id, i));
	}
    print_debug("Join time %.3fs\n", getTimeStamp() - startJoinTime);
    print_debug("table size : %lu %lu\n", table1.size(), table2.size());
	return simPairs;
}

Similarity *SimTable::ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims) {
	if (sims.empty()) {
		cerr << " No Similarity in ChooseBestIndexColumn" << endl;
		return NULL;
	}
	int least_candidates_number = num_row_ + 1;
	Similarity *least_sim = NULL;
	for (auto &sim : sims) {
		int num_candidates = indexes[sim.colx]->calcPrefixListSize(query_row[sim.coly]);
		if (num_candidates < least_candidates_number) {
			least_candidates_number = num_candidates;
			least_sim = &sim;
		}
	}
	if (query_row[0].id % 1000 == 0)
		print_debug("choose column %d\n", least_sim->colx);
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

double total_index_filter_time = 0.0;
double total_verify_time = 0.0;
vector<RowID> SimTable::Search(Row &query_row, vector<Similarity> &sims) {
    vector<int> candidateIDs;
    double time = getTimeStamp();

#ifdef INTERSECT_PREFIX_LIST
	unordered_set<int> new_candidates;
	for (int i = 0; i < int(sims.size()); ++i) {
		unordered_set<int> candidates = std::move(indexes[sims[i].colx]->getPrefixList(query_row[sims[i].coly]));
		if (i == 0)
			new_candidates = candidates;
		else
			new_candidates = intersect2Sets(new_candidates, candidates);
	}
	for (int id : new_candidates)
		candidateIDs.push_back(id);
#else
	Similarity *sim = ChooseBestIndexColumn(query_row, sims);
	auto new_candidates = indexes[sim->colx]->getPrefixList(query_row[sim->coly]);
	for (int id : new_candidates)
		candidateIDs.push_back(id);
	//sim->isSearched = true; // For default index, nothing has been searched
#endif
	double index_filter_time = getTimeStamp() - time;
    total_index_filter_time += index_filter_time;
	int queryId = query_row[0].id;
	if (queryId % 1000 == 0) {
		print_debug("id: %d, get prefix_list time: %.3fs %.3fs candidates.size() = %d\n",
				queryId,
				index_filter_time,
				total_index_filter_time / queryId,
				int(candidateIDs.size()));
		print_debug("%s\n", query_row[sim->coly].str.c_str());
	}
	time = getTimeStamp();

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
		print_debug("Verify time: %.3fs %.3fs\n", verify_time, total_verify_time / queryId);
	return result;
}

Estimation SimTable::Estimate(const vector<Field*> &column,
							const Field &query,
							Similarity &sim,
							const vector<int> &ids,
							Filter *filter) {
	static vector<int> sampleIDs;
	if (sampleIDs.empty()) {
		for (int i = 0; i < ids.size() * SAMPLE_RATIO; ++i)
			sampleIDs.push_back(ids[rand() % ids.size()]);
	}
	int pass = 0;
	int startTime = getTimeStamp();
	for (int id : sampleIDs) {
		pass += filter->filter(*column[id], query, sim);
    }
	double cost = double(getTimeStamp() - startTime) / sampleIDs.size();
	double ratio = 1.0 - double(pass) / sampleIDs.size();
	return Estimation(ratio, cost, filter, &sim);
}
