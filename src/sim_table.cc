#include "sim_table.h"
#include "common.h"
#include <unordered_map>
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
SimTable::SimTable(Table &table) {
	Init(table);
}
void SimTable::InitIndex(vector<Similarity> &sims) {
	cout << "Start init index" << endl;
	// Install index plugin, default is prefix_index
	indexes.clear();
	SimIndexFactory::InstallIndex();
	for (int c = 0; c < col_num_; ++c)
		indexes.push_back(SimIndexFactory::GetIndex()->GetInstance());
	for (auto &sim : sims)
		indexes[sim.colx]->build(column_table_[sim.colx], &sim);
	cout << "End init index" << endl;
}
void SimTable::Init(Table &table) {
	initFilters();
// TODO: currently no need for row matrix
//	table_ = table;
	row_num_ = table.size();
	col_num_ = table[0].size();
	cout << "start transpose !" << endl;
	transpose(table, &column_table_);
	cout << "transpose successfully!" << endl;
}
vector<pair<RowID, RowID>> SimTable::Join(Table &table1, Table &table2, vector<Similarity> &sims) {
	Init(table1);
	InitIndex(sims);
	vector<pair<RowID, RowID>> simPairs;
	startJoinTime = getTimeStamp();
	for (unsigned i = 0; i < table2.size(); ++i) {
		vector<RowID> results = Search(table2[i], sims);
		for (int id : results)
			simPairs.push_back(make_pair(id, i));
	}
	return simPairs;
}
Similarity *SimTable::ChooseBestIndexColumn(Row &query_row, vector<Similarity> &sims) {
	if (sims.empty()) {
		cerr << " No Similarity in ChooseBestIndexColumn" << endl;
		return NULL;
	}
	int least_candidates_number = row_num_ + 1;
	Similarity *least_sim = NULL;
	for (auto &sim : sims) {
		vector<int> candidateIDs;
		int startTime = getTimeStamp();
		// TODO: change index search to estimate
		indexes[sim.colx]->search(query_row[sim.coly], &candidateIDs);
#ifdef STAT
		int searchTime = getTimeStamp() - startTime;
		cout << "index searchTime = " << runTime / 1000.0 << "ms" << endl;
#endif
		if ((int)candidateIDs.size() < least_candidates_number) {
			least_candidates_number = candidateIDs.size();
			least_sim = &sim;
		}
	}
//	cout << "choose column " << least_sim.colx << endl;
	return least_sim;
}
vector<RowID> SimTable::Search(Row &query_row, vector<Similarity> &sims) {
	Similarity *sim = ChooseBestIndexColumn(query_row, sims);
	vector<int> candidateIDs;
	indexes[sim->colx]->search(query_row[sim->coly], &candidateIDs);
	sim->isSearched = true;

	vector<RowID> result;
	switch (FLAGS_exp_version) {
		case 0 : { result = Search0_NoEstimate(query_row, sims, candidateIDs); break; }
		case 1 : { result = Search1_Estimate(query_row, sims, candidateIDs); break; }
		case 2 : { result = Search2_TuneEstimate(query_row, sims, candidateIDs); break; }
		default: {
			cerr << "Error: NonExist exp_version, exp_version is in [" << 0 << ", " << 2 << "]" << endl;
			result = vector<RowID>(0);
		}
	}
	return result;
}
Estimation SimTable::Estimate(const Column &column,
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
	for (int id : sampleIDs)
		pass += filter->filter(column[id], query, sim);
	double cost = double(getTimeStamp() - startTime) / sampleIDs.size();
	double ratio = 1.0 - double(pass) / sampleIDs.size();
	return Estimation(ratio, cost, filter, &sim);
}
