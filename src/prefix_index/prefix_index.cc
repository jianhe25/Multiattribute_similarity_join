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
		int prefixlength = CalcPrefixLength(*field, *sim);
		//print_debug("prefixlength = %d\n",prefixlength);

		int len = tokens.size();
		for (int i = 0; i < prefixlength; ++i) {
			index_[tokens[i]][len].push_back(field->id);
		}
		indexSize_ += prefixlength;
	}
}

int PrefixIndex::calcPrefixListSize(Field &query) {
	int prefixlength = CalcPrefixLength(query, *sim_);
    if (prefixlength == 0) {
		assert(0);
		return fields_->size();
	} else {
		pair<int,int> length_bound = ComputeLengthBound(query, *sim_);
		vector<int> &tokens = query.tokens;
		int num_candidates = 0;
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end()) {
				auto &lists = index_[tokens[i]];
				// -1 means length_bound for this Similarity not known, thus take all
				if (length_bound.first == -1) {
					for (const auto &list : lists)
						num_candidates += list.second.size();
				} else {
					for (int i = length_bound.first; i <= length_bound.second; ++i) {
						num_candidates += lists[i].size();
					}
				}
			}
		}
		return num_candidates;
	}
}

vector<int> PrefixIndex::getPrefixList(Field &query) {
	vector<int> &tokens = query.tokens;
	int prefixlength = CalcPrefixLength(query, *sim_);
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
		pair<int,int> length_bound = ComputeLengthBound(query, *sim_);
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end()) {
				auto &lists = index_[tokens[i]];
				// -1 means length_bound for this Similarity not known, thus take all
				if (length_bound.first == -1) {
					for (const auto &list : lists) {
						for (int id : list.second)
							candidates.push_back(id);
					}
				} else {
					for (int i = length_bound.first; i <= length_bound.second; ++i) {
						const vector<int> &list = lists[i];
						for (int id : list)
							candidates.push_back(id);
					}
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
	//print_debug("#candidates: %d matchIDs: %d all: %d %d\n", int(candidates.size()), int(candidateIDs.size()), debugCandidatesNum, debugMatchidNum);
    verifyTime_ += getTimeStamp() - time;
	return candidateIDs;
}
