#include "../sim_table.h"
#include <algorithm>
using namespace std;

vector<RowID> SimTable::Search2_TuneEstimate(const Row &query_row,
										     vector<Similarity> &sims,
										     vector<int> &candidateIDs) {

	if (++verifyRound_ % FLAGS_estimate_filter_period == 0) {
		estimations_.clear();
		for (auto &sim : sims) {
			for (auto filter : g_filters) {
				if (sim.distType != ED && filter->Type() == "ContentFilter")
					continue;
				estimations_.push_back(Estimate(*tablePtr_, query_row[sim.coly],
							sim, candidateIDs, filter));
			}
		}
		sort(estimations_.begin(), estimations_.end());
	}

	//static int counter = 0;
	//if (++counter % 10000 == 0) {
		//for (const auto &estimation : estimations_) {
			//const auto &sim = estimation.sim;
			//print_debug("sim = %d %s %s %f %.15f\n", sim->colx, sim->type().c_str(), estimation.filter->Type().c_str(),
					//estimation.ratio, estimation.cost);
		//}
		//puts("");
	//}

	vector<int> result_ids;
	for (int id : candidateIDs) {
		//print_debug("id = %d\n", id);
		bool is_same = true;
		for (const auto &estimation : estimations_) {
			const auto &sim = estimation.sim;
			//print_debug("type = %s sim = %d pass = %d \n", estimation.filter->Type().c_str(), sim->colx,
					//estimation.filter->filter((*tablePtr_)[id][sim->colx], query_row[sim->coly], *sim));
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
