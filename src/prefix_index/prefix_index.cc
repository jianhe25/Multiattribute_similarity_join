#include "prefix_index.h"
#include <algorithm>
using namespace std;

int total_num_candidates_scanned = 0;
int total_num_large_index_candidates_scanned = 0;

void PrefixIndex::CalcTF(const vector<Field*> &fields, Similarity *sim) {
	for (auto field : fields) {
		for (int token : field->tokens)
			token_counter_[token]++;
	}
}

bool CompareList(const pair<int, vector<int>> &list1,
                 const pair<int, vector<int>> &list2) {
    return list1.second.size() > list2.second.size();
}

unordered_set<int> list_has_subtree;
void PrefixIndex::build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim) {
	sim_ = sim;
	fields_ = &fields1;

    // TODO: Put TF calculation and sort before build
	CalcTF(fields1, sim);

	for (auto field : fields1) {
		sort(field->tokens.begin(), field->tokens.end(), CompareTokenByTF(*this));
	}

	for (auto field : fields1) {
		vector<int> &tokens = field->tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), *sim);
		for (int i = 0; i < prefixlength; ++i)
			index_[tokens[i]].push_back(field->id);
	}

		/*
		 * Build recursive prefix index
		 * 1) Sort list by list_length
		 * 2) Select long list to split, and split by other attributes
		 *      2.1) Select list that has less repeated tokens.
		 *      2.2) number of new token after add list >= 0.7 * sum(list.size())
		 *      2.3) list should be long enough to split, otherwise is meaningless
		 */
		int total = 0;
		for (auto list : index_) {
			total += list.second.size();
		}
		vector<pair<int, vector<int>>> sorted_lists;
		for (auto list : index_) {
			sorted_lists.push_back(make_pair(list.first, list.second));
		}
		// sort list by list size, from large to small
		sort(sorted_lists.begin(), sorted_lists.end(), CompareList);

		int num_large_list = 0;
		int list_place = 0;
		unordered_map<int, int> tokenMap;
		for (const auto &list : sorted_lists) {
			int num_new_token = 0;
			for (int id : list.second) {
				if (tokenMap.find(id) == tokenMap.end()) {
					num_new_token++;
				}
			}
			// Split list
			if (num_new_token > 0.5 * int(list.second.size()) && list_place < int(sorted_lists.size())) {
				for (int id : list.second) {
					tokenMap[id]++;
				}
				num_large_list += list.second.size();
				list_has_subtree.insert(list.first);
			}
			++list_place;
		}
		cout << "tokenMap.size() = " << tokenMap.size() << " num_large_list = " << num_large_list << endl;

    //freopen("inverted.txt","w", stdout);
    //cout << "tokenNum = " << sizeArray.size() << endl;
    //sort(sizeArray.begin(), sizeArray.end());
    //for (int size : sizeArray) {
    //printf("%d %f\n",size, double(size) / total);
    //}

    // Sort by token value for verification
	for (auto field : fields1) {
		sort(field->tokens.begin(), field->tokens.end());
	}
}

int PrefixIndex::calcPrefixListSize(Field &query) {
	int prefixlength = CalcPrefixLength(query.tokens.size(), *sim_);
    if (prefixlength == 0) {
		return fields_->size();
	} else {
		vector<int> &tokens = query.tokens;
		sort(tokens.begin(), tokens.end(), CompareTokenByTF(*this));
		int num_candidates = 0;
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end()) {
				num_candidates += index_[ tokens[i] ].size();
			}
		}
		// Sort back by numbers
		sort(tokens.begin(), tokens.end());
		return num_candidates;
	}
}

unordered_set<int> PrefixIndex::getPrefixList(Field &query) {
	vector<int> &tokens = query.tokens;
	sort(tokens.begin(), tokens.end(), CompareTokenByTF(*this));
	int prefixlength = CalcPrefixLength(tokens.size(), *sim_);
	unordered_set<FieldID> candidates;
	/*
	 * prefixlength == 0 means no prefix can be used to filter candidates
	 */
	/* TODO: PrefixIndex is buggy, preset prefixlength = 0 would get more matchIDs than only use prefix. */
	//prefixlength = 0;
	double time = getTimeStamp();

    int num_candidates_scanned = 0;
    int num_large_index_candidates_scanned = 0;
    if (prefixlength == 0) {
		for (int fieldid = 0; fieldid < int(fields_->size()); ++fieldid)
			candidates.insert(fieldid);
	} else {
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end()) {
				for (int fieldid: index_[ tokens[i] ]) {
					candidates.insert(fieldid);
				}
				num_candidates_scanned += index_[ tokens[i] ].size();
				if (list_has_subtree.find(tokens[i]) != list_has_subtree.end()) {
					num_large_index_candidates_scanned += index_[tokens[i]].size();
				}
			}
		}
	}

	// Sort back by numbers
	sort(tokens.begin(), tokens.end());

    mergeListTime_ += getTimeStamp() - time;
    total_num_candidates_scanned += num_candidates_scanned;
    total_num_large_index_candidates_scanned += num_large_index_candidates_scanned;

    //cout << "num_candidates_scanned " << num_candidates_scanned << " num_candidates_verified = " << int(candidates.size())
        //<< " num_large_index_candidates_scanned = " << num_large_index_candidates_scanned << endl;

    //cout << "total_num_candidates_scanned = " << total_num_candidates_scanned
        //<< ", total_num_large_index_candidates_scanned = " << total_num_large_index_candidates_scanned << endl;
    //if (matchIDs->size() > 10) {
    //cout << endl;
    //cout << query.id << "  query = " << query.str << endl;
    //for (int id : *matchIDs) {
    //cout << id << " " << (*fields_)[id]->str << endl;
    //}
    //cout << ", Verify time " << verifyTime_ << endl;
    //}
    return candidates;
}


int debugCandidatesNum = 0;
int debugMatchidNum = 0;

void PrefixIndex::search(Field &query, vector<int> *matchIDs) {
    unordered_set<FieldID> candidates = std::move( getPrefixList(query) );
    double time = getTimeStamp();
	for (auto fieldid : candidates) {
		if (verifier_.filter(query, *(*fields_)[fieldid], *sim_))
			matchIDs->push_back(fieldid);
	}
    debugCandidatesNum += candidates.size();
    debugMatchidNum += matchIDs->size();
	//print_debug("#candidates: %d %d %d %d\n", int(candidates.size()), int(matchIDs->size()), debugCandidatesNum, debugMatchidNum);
    verifyTime_ += getTimeStamp() - time;
}
