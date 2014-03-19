#include "../sim_table.h"
#include <algorithm>
using namespace std;

vector<RowID> SimTable::Search2_TuneEstimate(const Row &query_row,
										     vector<Similarity> &sims,
										     vector<int> &candidateIDs) {
	vector<Estimation> estimations;
	for (auto &sim : sims) {
		if (sim.isSearched) continue;
		for (auto filter : g_filters) {
			estimations.push_back(Estimate(column_table_[sim.colx], query_row[sim.coly],
				sim, candidateIDs, filter));
		}
	}

	sort(estimations.begin(), estimations.end());

	/*
	 * First round filter
	 */
	for (auto &estimation : estimations) {
		auto sim = estimation.sim;
		Column &column = column_table_[sim->colx];
		vector<int> ids;
		for (int id : candidateIDs) {
			if (estimation.filter->filter(column[id], query_row[sim->coly], *sim))
				ids.push_back(id);
		}
		candidateIDs = ids;
	}
	/*
	 * Verify candidates
	Verifier *verifier = new Verifier();
	for (auto &estimation : estimations)
	if (estimation.filter->EchoType() != "Verifier") {
		auto sim = estimation.sim;
		Column &column = column_table_[sim->colx];
		vector<int> ids;
		for (int id : candidateIDs) {
			if (verifier->filter(column[id], query_row[sim->coly], *sim))
				ids.push_back(id);
		}
		candidateIDs = ids;
	}
	*/
	return candidateIDs;
}
