#include "../sim_table.h"

#ifdef STAT
static int idsLeft = 0;
static int search_round = 0;
#endif

vector<RowID> SimTable::Search0_NoEstimate(const Row &query_row,
										   vector<Similarity> &sims,
										   vector<int> &candidateIDs) {
	/*
	 * Verify candidates
	 */
    vector<int> matchIDs;
    for (int id : candidateIDs) {
        bool isMatch = true;
        for (auto sim : sims) {
			if (!verifier_.filter((*tablePtr_)[id][sim.colx], query_row[sim.coly], sim)) {
                isMatch = false;
                break;
            }
        }
        if (isMatch)
            matchIDs.push_back(id);
    }
    return matchIDs;
	//for (auto &sim : sims) {
		//Column &column = column_table_[sim.colx];
		//vector<int> leftIDs;
		//for (int id : candidateIDs) {
			//if (verifier->filter(column[id], query_row[sim.coly], sim))
				//leftIDs.push_back(id);
		//}
//#ifdef STAT
		//++search_round;
		//idsLeft += leftIDs.size();
		//cout << "column = " << sim.colx << " .. = " << leftIDs.size() << endl;
		//cout << endl;
//#endif
		//candidateIDs = leftIDs;
	//}
    //return candidateIDs;
}

