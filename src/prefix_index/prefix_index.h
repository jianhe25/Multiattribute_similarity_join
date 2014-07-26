#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gflags/gflags.h>

#include "../common.h"
#include "../core.h"
#include "../filter.h"
using namespace std;

class PrefixIndex {
public:
	typedef int Token;
	typedef int FieldID;
	unordered_map<int, int> token_counter_;
	unordered_map<int, vector<FieldID> > index_;
    Verifier verifier_;

	Similarity *sim_;

	vector<Field*> *fields_;
	int indexSize_;

	void CalcTF(const vector<Field*> &fields, Similarity *sim);

	struct CompareTokenByTF {
		const PrefixIndex& prefixindex;
		CompareTokenByTF(const PrefixIndex &_prefixindex) : prefixindex(_prefixindex) {}
		bool operator()(int a, int b) {
			const unordered_map<int, int> &token_counter = prefixindex.token_counter_;
			const auto it1 = token_counter.find(a);
			const auto it2 = token_counter.find(b);
			if (it1 == token_counter.end() && it2 == token_counter.end()) {
				return a < b;
			} else if (it1 == token_counter.end()) {
				return true;
			} else if (it2 == token_counter.end()) {
				return false;
			} else {
				return it1->second < it2->second || (it1->second == it2->second && a < b);
			}
		}
	};
	void build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim);

    unordered_set<int> getPrefixList(Field &query);
	int calcPrefixListSize(Field &query);

	vector<int> search(Field &query);

    double mergeListTime_;
    double verifyTime_;
};

