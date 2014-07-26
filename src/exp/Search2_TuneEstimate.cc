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
			if (sim.distType == JACCARD && filter->Type() == "ContentFilter")
				continue;
			estimations.push_back(Estimate(column_table1_[sim.colx], query_row[sim.coly],
										   sim, candidateIDs, filter));
		}
	}
	sort(estimations.begin(), estimations.end());

	static int counter = 0;
	if (++counter % 10000 == 0) {
		for (const auto &estimation : estimations) {
			const auto &sim = estimation.sim;
			print_debug("sim = %d %s %s %f %.15f\n", sim->colx, sim->type().c_str(), estimation.filter->Type().c_str(),
					estimation.ratio, estimation.cost);
		}
		puts("");
	}

	vector<int> result_ids;
	for (int id : candidateIDs) {
		bool is_same = true;
		for (auto &estimation : estimations) {
			const auto &sim = estimation.sim;
			if (!estimation.filter->filter((*tablePtr_)[id][sim->colx], query_row[sim->coly], *sim)) {
				is_same = false;
				break;
			}
		}
		if (is_same)
			result_ids.push_back(id);
	}
	return result_ids;

	/*
	 * Exp2 : Filter without estimation
	 */
	/*
	vector<int> result_ids;
	for (int id : candidateIDs) {
		bool is_same = true;
		for (const auto &filter : g_filters) {
			for (const auto &sim : sims) {
				if (!filter->filter((*tablePtr_)[id][sim.colx], query_row[sim.coly], sim)) {
					is_same = false;
					break;
				}
			}
			if (!is_same) break;
		}
		if (is_same)
			result_ids.push_back(id);
	}
	return result_ids;
	*/
}
