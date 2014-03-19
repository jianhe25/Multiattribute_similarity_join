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
int PrefixIndex::CalcPrefixLength(int size, Similarity *sim) {
	double tau = sim->dist;
	int common = -1;
	if (sim->distType == JACCARD)
		common = size * tau / (1.0 + tau);
	else if (sim->distType == COSINE)
		common = sqrt(double(size)) * tau;
	else if (sim->distType == DICE)
		common = size * tau / 2.0;
	else if (sim->distType == ED)
		common = max(0, size - (int)(tau * GRAM_LENGTH));
	else {
		cerr << "Non exist Similarity type when CalcPrefixLength\n";
		assert(0);
		return -1;
	}
	return size - max(common-1, 0);
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
	CalcTF(fields, sim);
	for (auto &field : fields) {
		vector<int> &tokens = field.tokens;
		sort(tokens.begin(), tokens.end(), CompareTokenByTF(*this));
	}
	for (auto &field : fields) {
		vector<int> &tokens = field.tokens;
		int prefixlength = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefixlength; ++i)
			index_[tokens[i]].push_back(field.id);
	}
}
void PrefixIndex::search(Field &query, vector<int> *matchIDs) {
	vector<int> &tokens = query.tokens;
	sort(tokens.begin(), tokens.end(), CompareTokenByTF(*this));
	int prefixlength = CalcPrefixLength(tokens.size(), sim_);
	unordered_set<FieldID> candidates;
	/*
	 * prefixlength == 0 means no prefix can be used to filter candidates
	 */
	/* TODO: PrefixIndex is buggy, preset prefixlength = 0 would get more matchIDs than only use prefix. */
	//prefixlength = 0;
	if (prefixlength == 0) {
		for (int fieldid = 0; fieldid < int(fields_->size()); ++fieldid)
			candidates.insert(fieldid);
	} else {
		for (int i = 0; i < prefixlength; ++i) {
			if (index_.find(tokens[i]) != index_.end())
				for (int fieldid: index_[ tokens[i] ]) {
					candidates.insert(fieldid);
				}
		}
	}
	Verifier *verifier = new Verifier();
	for (auto fieldid : candidates) {
		if (verifier->filter(query, (*fields_)[fieldid], *sim_))
			matchIDs->push_back(fieldid);
	}
}
PrefixIndex* PrefixIndex::GetInstance() {
	return new PrefixIndex();
}


