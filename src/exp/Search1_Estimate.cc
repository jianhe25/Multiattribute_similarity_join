#include "../sim_table.h"
#include <algorithm>
using namespace std;

vector<RowID> SimTable::Search1_Estimate(const Row &query_row,
										 vector<Similarity> &sims,
										 vector<int> &candidateIDs) {
	vector<Estimation> estimations;
	Verifier *verifier = new Verifier();
	for (auto &sim : sims) {
		estimations.push_back(Estimate(column_table_[sim.colx], query_row[sim.coly],
			sim, candidateIDs, verifier));
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
	return candidateIDs;
}
