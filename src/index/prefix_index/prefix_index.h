#include "../index.h"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
using namespace std;

/* PrefixIndex is buggy, preset prefixlength = 0 would get more matchIDs than only use prefix. */
class PrefixIndex : public SimIndex {
public:
	typedef int Token;
	typedef int FieldID;
	unordered_map<int, int> token_counter_;
	unordered_map<int, vector<FieldID> > index_;

	void CalcTF(const vector<Field> &fields, Similarity *sim);

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
	void build(vector<Field> &fields, Similarity *sim);
	void search(Field &query, vector<int> *matchIDs);
	PrefixIndex *GetInstance();

    double mergeListTime_;
    double verifyTime_;
};

