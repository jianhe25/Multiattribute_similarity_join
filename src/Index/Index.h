#include "../src/common.h"

class SimIndex {

	SimIndex();

	void build(const vector<string> &strings) = 0;

	void search(string query, Similarity similarity, vector<int> *ids) = 0;
};


