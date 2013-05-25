#ifndef SRC_FILTER
#define SRC_FILTER

#include <string>
#include <vector>
#include "core.h"
#include "common.h"
using namespace std;

class Filter {
public:
	virtual bool filter(const Field &a, const Field &b, const Similarity &sim) = 0;
	virtual string EchoType();
};

class Verifier : public Filter {
	bool VerifyOverlapToken(const vector<int> tokensA, const vector<int> tokensB, int needOverlap);
	bool VerifyEditDistance(const string &a, const string &b, int dist);
	int edit_distance_;
	int overlap_;
public:
	string EchoType();
	bool filter(const Field &a, const Field &b, const Similarity &sim);
	int edit_distance();
	int overlap();
};

class LengthFilter : public Filter {
public:
	string EchoType();
	bool filter(const Field &a, const Field &b, const Similarity &sim);
};

class ContentFilter : public Filter {
public:
	string EchoType();
	bool filter(const Field &a, const Field &b, const Similarity &sim);
};

extern vector<Filter*> g_filters;

void initFilters();
void registerFilter(Filter *filter);

#endif // SRC_FILTER

