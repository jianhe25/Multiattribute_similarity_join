#include "prefix_index.h"
#include <algorithm>
using namespace std;

void PrefixIndex::CalcTF(const vector<Field> &fields, Similarity *sim) {
	for (auto &field : fields) {
		const vector<int> &tokens = field.tokens;
		for (int token : tokens)
			token_counter_[token]++;
	}
}
void PrefixIndex::build(vector<Field> &fields, Similarity *sim) {
	sim_ = sim;
	fields_ = &fields;
	//if (sim->distType == ED) {
		//for (auto &field : fields)
			//if (field.grams.empty()) {
				//field.GenerateGrams();
			//}
	//}

    // TODO: Put TF calculation and sort before build
	CalcTF(fields, sim);
	for (auto &field : fields) {
		vector<int> &tokens = field.tokens;
		sort(tokens.begin(), tokens.end(), CompareTokenByTF(*this));
	}
	for (auto &field : fields) {
		vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), *sim);
		for (int i = 0; i < prefixlength; ++i)
			index_[tokens[i]].push_back(field.id);
	}
}
void PrefixIndex::search(Field &query, vector<int> *matchIDs) {
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
    if (prefixlength == 0) {
		for (int fieldid = 0; fieldid < int(fields_->size()); ++fieldid)
			candidates.insert(fieldid);
	} else {
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end())
				for (int fieldid: index_[ tokens[i] ]) {
					candidates.insert(fieldid);
				}
            num_candidates_scanned += index_[ tokens[i] ].size();
		}
	}
    mergeListTime_ += getTimeStamp() - time;
    //cout << "MergeList time " << mergeListTime_ << " Scanned: " << num_candidates_scanned << " Verified: " << candidates.size();
    time = getTimeStamp();
	Verifier *verifier = new Verifier();
	for (auto fieldid : candidates) {
		if (verifier->filter(query, (*fields_)[fieldid], *sim_))
			matchIDs->push_back(fieldid);
	}
    verifyTime_ += getTimeStamp() - time;
    //cout << ", Verify time " << verifyTime_ << endl;
}
PrefixIndex* PrefixIndex::GetInstance() {
	return new PrefixIndex();
}


