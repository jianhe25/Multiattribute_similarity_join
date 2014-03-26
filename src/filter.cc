#include "filter.h"
#include <algorithm>
#include <iostream>

string Filter::EchoType() {
	return "NULL";
}
string Verifier::EchoType() {
	return "Verifier";
}
int Verifier::edit_distance() {
	return edit_distance_;
}
int Verifier::overlap() {
	return overlap_;
}
bool Verifier::VerifyEditDistance(const string &a, const string &b, int dist) {
	const int LIMIT = dist + 1;
	vector<vector<int>> dp(a.length()+1, vector<int>(b.length()+1, LIMIT));
	dp[0][0] = 0;
	for (int i = 0; i < (int)a.length(); ++i) {
		int l = max(0, i-dist);
		int r = min((int)b.length()-1, i+dist);
		bool isUpdate = false;
		for (int j = l; j <= r; ++j)
		if (dp[i][j] != LIMIT) {
			dp[i][j+1] = min(dp[i][j+1], dp[i][j] + 1);
			dp[i+1][j] = min(dp[i+1][j], dp[i][j] + 1);
			dp[i+1][j+1] = min(dp[i+1][j+1], dp[i][j] + (a[i] != b[j]));
			isUpdate = true;
		}
		if (!isUpdate) {
			edit_distance_ = LIMIT;
			return false;
		}
	}
	edit_distance_ = dp[a.length()][b.length()];
	return edit_distance_ <= dist;
}
// TODO : add early termination
bool Verifier::VerifyOverlapToken(const vector<int> tokensA, const vector<int> tokensB, int needOverlap) {
	overlap_ = 0;
	int j = 0;
	for (int i = 0; i < (int)tokensA.size(); ++i) {
		while (j < (int)tokensB.size() && tokensA[i] > tokensB[j])
			++j;
		if (j < (int)tokensB.size()) {
			if (tokensA[i] == tokensB[j]) {
				overlap_++;
				++j;
			}
		}
	}
	return overlap_ >= needOverlap;
}

bool Verifier::filter(const Field &a, const Field &b, const Similarity &sim) {
	bool isMatch = false;
	if (sim.distType == ED) {
		isMatch = VerifyEditDistance(a.str, b.str, sim.dist);
        //cout << "Verify " << a.str << "----------" << b.str << " " << result << " ED = " << edit_distance_ << endl;
	} else {
		int overlap = CalcOverlap(a.tokens.size(), b.tokens.size(), sim);
		isMatch = VerifyOverlapToken(a.tokens, b.tokens, overlap);
		//cout << "Verify " << a.str << "----------" << b.str << " " << result << " o = " << overlap << endl;
	}
	return isMatch;
}

string LengthFilter::EchoType() {
	return "LengthFilter";
}
bool LengthFilter::filter(const Field &a, const Field &b, const Similarity &sim) {
	bool result = false;
	if (sim.distType == ED)
		result = (abs(int(a.str.length() - b.str.length())) <= (int)sim.dist);
	else {
		int overlap = CalcOverlap(a.tokens.size(), b.tokens.size(), sim);
		result = ((int)a.tokens.size() >= overlap && (int)b.tokens.size() >= overlap);
	}
//	cout << "LengthFilter " << a.str.length() << "---------------" << b.str.length() << " " << result << " " << sim.dist << endl;
	return result;
}

string ContentFilter::EchoType() {
	return "ContentFilter";
}
bool ContentFilter::filter(const Field &a, const Field &b, const Similarity &sim) {
	bool result = false;
	if (sim.distType == ED)
		result = (abs(int(a.str.length() - b.str.length())) <= (int)sim.dist);
	else {
		int overlap = CalcOverlap(a.tokens.size(), b.tokens.size(), sim);
		result = ((int)a.tokens.size() >= overlap && (int)b.tokens.size() >= overlap);
	}
//	cout << "LengthFilter " << a.str.length() << "---------------" << b.str.length() << " " << result << " " << sim.dist << endl;
	return result;
}

vector<Filter*> g_filters;
void initFilters() {
	g_filters.clear();
	registerFilter(new Verifier());
	registerFilter(new LengthFilter());
}
void registerFilter(Filter *filter) {
	g_filters.push_back(filter);
}


