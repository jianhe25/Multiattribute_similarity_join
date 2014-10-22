#include "prefix_index.h"
#include <algorithm>
using namespace std;

int total_num_candidates_scanned = 0;
int total_num_large_index_candidates_scanned = 0;

bool CompareList(const pair<int, vector<int>> &list1,
                 const pair<int, vector<int>> &list2) {
    return list1.second.size() > list2.second.size();
}

void PrefixIndex::build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim) {
	sim_ = *sim;
	print_debug("sim->distType = %d\n", sim_.distType);
	fields_ = &fields1;
	indexSize_ = 0;
	bool is_ppjoin = (FLAGS_baseline_exp == "edjoin+ppjoin");
	for (auto field : fields1) {
		vector<int> &tokens = field->tokens;
		int prefixlength = CalcPrefixLength(*field, sim_);
		//print_debug("prefixlength = %d\n",prefixlength);
		int len = tokens.size();
		for (int i = 0; i < prefixlength; ++i) {
			if (is_ppjoin) {
				positionedIndex_[tokens[i]][len].push_back(make_pair(field->id, i));
			} else {
				index_[tokens[i]][len].push_back(field->id);
			}
		}
		indexSize_ += prefixlength;
	}
}

int PrefixIndex::calcPrefixListSize(Field &query, const Similarity &sim) {
	int prefixlength = CalcPrefixLength(query, sim);
	bool is_ppjoin = (FLAGS_baseline_exp == "edjoin+ppjoin");
    if (prefixlength == 0) {
		assert(0);
		return fields_->size();
	} else {
		pair<int,int> length_bound = ComputeLengthBound(query, sim);
		vector<int> &tokens = query.tokens;
		int num_candidates = 0;
		for (int i = 0; i < prefixlength; ++i) {
			if (is_ppjoin) {
				if (positionedIndex_.find(tokens[i]) != positionedIndex_.end()) {
					auto &lists = positionedIndex_[tokens[i]];
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
			} else {
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
		}
		return num_candidates;
	}
}
bool PrefixIndex::ppjoinCheck(const Field &a, int pos1, const Field &b, int pos2) {
	//print_debug("%d %d %d %d %d\n",a.tokens.size(), pos1, b.tokens.size(), pos2, CalcOverlap(a.tokens.size(), b.tokens.size(), *sim_));
	bool result = min(a.tokens.size() - pos1, b.tokens.size() - pos2) >= CalcOverlap(a.tokens.size(), b.tokens.size(), sim_);
	return result;
}
vector<int> PrefixIndex::getPrefixList(Field &query, const Similarity &sim) {
	vector<int> &tokens = query.tokens;
	int prefixlength = CalcPrefixLength(query, sim);
	vector<int> candidates;
	/*
	 * prefixlength == 0 means no prefix can be used to filter candidates
	 */
	/* TODO: PrefixIndex is buggy, preset prefixlength = 0 would get more matchIDs than only use prefix. */
	//prefixlength = 0;
	double time = getTimeStamp();

    if (prefixlength == 0) {
		assert(0); // cannot happen
		for (int fieldid = 0; fieldid < int(fields_->size()); ++fieldid)
			candidates.push_back(fieldid);
	} else {
		pair<int,int> length_bound = ComputeLengthBound(query, sim);
		if (FLAGS_baseline_exp == "edjoin+ppjoin") {
			for (int i = 0; i < prefixlength; ++i) {
				if (positionedIndex_.find(tokens[i]) != positionedIndex_.end()) {
					auto &lists = positionedIndex_[tokens[i]];
					for (int len = length_bound.first; len <= length_bound.second; ++len) {
						const vector<pair<int,int>> &list = lists[len];
						for (pair<int,int> id_pos : list) {
							if (ppjoinCheck(*(*fields_)[id_pos.first], id_pos.second, query, i))
								candidates.push_back(id_pos.first);
						}
					}
				}
			}
		} else {
			for (int i = 0; i < prefixlength; ++i) {
				if (index_.find(tokens[i]) != index_.end()) {
					auto &lists = index_[tokens[i]];
					for (int len = length_bound.first; len <= length_bound.second; ++len) {
						const vector<int> &list = lists[len];
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

vector<int> PrefixIndex::search(Field &query, const Similarity &sim) {
    vector<FieldID> candidates = std::move( getPrefixList(query, sim) );

	sort(candidates.begin(), candidates.end());
	candidates.erase(unique(candidates.begin(), candidates.end()), candidates.end());

    double time = getTimeStamp();
	vector<int> candidateIDs;
	for (auto fieldid : candidates) {
		if (verifier_.filter(query, *(*fields_)[fieldid], sim))
			candidateIDs.push_back(fieldid);
	}

    debugCandidatesNum += candidates.size();
    debugMatchidNum += candidateIDs.size();
	//print_debug("#candidates: %d matchIDs: %d all: %d %d\n", int(candidates.size()), int(candidateIDs.size()), debugCandidatesNum, debugMatchidNum);
    verifyTime_ += getTimeStamp() - time;
	return candidateIDs;
}
