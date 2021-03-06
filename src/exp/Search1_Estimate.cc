#include "../sim_table.h"
#include <algorithm>
using namespace std;

vector<RowID> SimTable::Search1_Estimate(const Row &query_row,
										 vector<Similarity> &sims,
										 vector<int> &candidateIDs) {

	if (++verifyRound_ % FLAGS_estimate_filter_period == 0) {
		estimations_.clear();
		for (auto &sim : sims) {
			estimations_.push_back(Estimate(*tablePtr_, query_row[sim.coly],
				sim, candidateIDs, &verifier_));
		}
		sort(estimations_.begin(), estimations_.end());
	}
	/*
	 * First round filter
	 */
    //cout << "Before Estimation " << query_row[0].id << endl;
    //for (int id : candidateIDs) cout << id << " "; cout << endl;


	vector<int> result_ids;
	for (int id : candidateIDs) {
		bool is_same = true;
		for (auto &estimation : estimations_) {
			const auto &sim = estimation.sim;
			for (auto filter : g_filters) {
				if (sim->distType != ED && filter->Type() == "ContentFilter")
					continue;
				if (!filter->filter((*tablePtr_)[id][sim->colx], query_row[sim->coly], *sim)) {
					is_same = false;
					break;
				}
			}
			if (!is_same) break;
			//if (!estimation.filter->filter((*tablePtr_)[id][sim->colx], query_row[sim->coly], *sim)) {
			//is_same = false;
			//break;
			//}
		}
		if (is_same)
			result_ids.push_back(id);
	}
	return result_ids;
}
