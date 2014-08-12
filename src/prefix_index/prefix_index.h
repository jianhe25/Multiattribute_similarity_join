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

	void build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim);

    vector<int> getPrefixList(Field &query);
	int calcPrefixListSize(Field &query);

	vector<int> search(Field &query);

    double mergeListTime_;
    double verifyTime_;
};

