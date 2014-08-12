#include "prefix_index.h"
#include <algorithm>
using namespace std;

int total_num_candidates_scanned = 0;
int total_num_large_index_candidates_scanned = 0;

bool CompareList(const pair<int, vector<int>> &list1,
                 const pair<int, vector<int>> &list2) {
    return list1.second.size() > list2.second.size();
}

unordered_set<int> list_has_subtree;
void PrefixIndex::build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim) {
	sim_ = sim;
	fields_ = &fields1;

	indexSize_ = 0;
	for (auto field : fields1) {
		vector<int> &tokens = field->tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), *sim);
		for (int i = 0; i < prefixlength; ++i)
			index_[tokens[i]].push_back(field->id);
		indexSize_ += prefixlength;
	}
}

int PrefixIndex::calcPrefixListSize(Field &query) {
	int prefixlength = CalcPrefixLength(query.tokens.size(), *sim_);
    if (prefixlength == 0) {
		return fields_->size();
	} else {
		vector<int> &tokens = query.tokens;
		int num_candidates = 0;
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end()) {
				num_candidates += index_[ tokens[i] ].size();
			}
		}
		return num_candidates;
	}
}

vector<int> PrefixIndex::getPrefixList(Field &query) {
	vector<int> &tokens = query.tokens;
	int prefixlength = CalcPrefixLength(tokens.size(), *sim_);
	vector<int> candidates;
	/*
	 * prefixlength == 0 means no prefix can be used to filter candidates
	 */
	/* TODO: PrefixIndex is buggy, preset prefixlength = 0 would get more matchIDs than only use prefix. */
	//prefixlength = 0;
	double time = getTimeStamp();

    if (prefixlength == 0) {
		for (int fieldid = 0; fieldid < int(fields_->size()); ++fieldid)
			candidates.push_back(fieldid);
	} else {
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end()) {
				for (int fieldid: index_[ tokens[i] ]) {
					candidates.push_back(fieldid);
				}
			}
		}
	}
    mergeListTime_ += getTimeStamp() - time;
    return candidates;
}


int debugCandidatesNum = 0;
int debugMatchidNum = 0;

vector<int> PrefixIndex::search(Field &query) {
    vector<FieldID> candidates = std::move( getPrefixList(query) );
	sort(candidates.begin(), candidates.end());
	candidates.erase(unique(candidates.begin(), candidates.end()), candidates.end());
    double time = getTimeStamp();
	vector<int> candidateIDs;
	for (auto fieldid : candidates) {
		if (verifier_.filter(query, *(*fields_)[fieldid], *sim_))
			candidateIDs.push_back(fieldid);
	}
    debugCandidatesNum += candidates.size();
    debugMatchidNum += candidateIDs.size();
	//print_debug("#candidates: %d %d %d %d\n", int(candidates.size()), int(matchIDs->size()), debugCandidatesNum, debugMatchidNum);
    verifyTime_ += getTimeStamp() - time;
	return candidateIDs;
}
