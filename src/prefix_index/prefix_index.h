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
	unordered_map<int, unordered_map<int, vector<int> > > index_;
	unordered_map<int, unordered_map<int, vector<pair<int, int>> > > positionedIndex_;
    Verifier verifier_;

	Similarity sim_;

	vector<Field*> *fields_;
	int indexSize_;

	void build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim);

    vector<int> getPrefixList(Field &query, const Similarity &sim);
	int calcPrefixListSize(Field &query, const Similarity &sim);

	vector<int> search(Field &query, const Similarity &sim);
	bool ppjoinCheck(const Field &a, int pos1, const Field &b, int pos2);

    double mergeListTime_;
    double verifyTime_;
};

