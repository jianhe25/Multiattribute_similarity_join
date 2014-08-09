#include "filter.h"
#include <algorithm>
#include <iostream>

string Filter::Type() {
	return "NULL";
}
string Verifier::Type() {
	return "Verifier";
}
int Verifier::edit_distance() {
	return edit_distance_;
}
int Verifier::overlap() {
	return overlap_;
}
bool Verifier::VerifyEditDistance(const string &a, const string &b, int dist) {
	int LIMIT = dist + 1;
    //cout << "a = " << a << endl;
    //cout << "b = " << b << endl;
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
bool Verifier::VerifyOverlapToken(const vector<int> tokensA, const vector<int> tokensB, int needOverlap) {
	overlap_ = 0;
	int j = 0;
	for (int i = 0; i < (int)tokensA.size(); ++i) {
		while (j < (int)tokensB.size() && tokensB[j] < tokensA[i])
			++j;
		if (j < (int)tokensB.size()) {
			if (tokensA[i] == tokensB[j]) {
				overlap_++;
				++j;
			}
		}
		// Early termination
		if (int(tokensA.size()) - i - 1 + overlap_ < needOverlap || int(tokensB.size()) - j + overlap_ < needOverlap)
			return false;
	}
	return overlap_ >= needOverlap;
}

bool Verifier::filter(const Field &a, const Field &b, const Similarity &sim) {
	bool isMatch = false;
	if (sim.distType == ED || sim.distType == ES) {
		int ed = sim.distType == ED? sim.dist : ceil((1 - sim.dist) * max(a.str.length(), b.str.length()));
		isMatch = VerifyEditDistance(a.str, b.str, ed);
		//if (a.id == b.id)
		//cout << "VerifyED " << a.str << "----------" << b.str << " match=" << isMatch << " ED = " << edit_distance_ << " tau = " << ed << endl;
	} else if (sim.distType == JACCARD ||
			   sim.distType == COSINE ||
			   sim.distType == DICE ||
			   sim.distType == OLP) {
		int needOverlap = CalcOverlap(a.tokens.size(), b.tokens.size(), sim);
		isMatch = VerifyOverlapToken(a.tokens, b.tokens, needOverlap);
		//if (a.id == b.id) {
		//for (int id : a.tokens) cout << id << " "; cout << endl;
		//for (int id : b.tokens) cout << id << " "; cout << endl;
		//cout << "VerifyOverlap " << a.str << "----------" << b.str << " match=" << isMatch << " o = " << needOverlap << " " << a.tokens.size() << " " << b.tokens.size() << endl;
		//}
	} else {
		print_debug("Error: Non exist distType\n");
	}
	return isMatch;
}

string LengthFilter::Type() {
	return "LengthFilter";
}

bool LengthFilter::filter(const Field &a, const Field &b, const Similarity &sim) {
	bool result = false;
	int lenA = a.tokens.size();
	int lenB = b.tokens.size();
	double tau = sim.dist;
	if (sim.distType == ES) {
		int ed = ceil((1 - tau) * max(a.str.length(), b.str.length()));
		result = (abs(int(a.str.length() - b.str.length())) <= ed);
	} else if (sim.distType == ED)
		result = (abs(int(a.str.length() - b.str.length())) <= (int)tau);
	else if (sim.distType == JACCARD) {
		result = (lenA >= ceil(tau * lenB)) &&
				 (lenA <= floor(lenB / tau));
	} else if (sim.distType == COSINE) {
		result = (lenA >= ceil(tau * tau * lenB)) &&
				 (lenA <= floor(lenB / tau / tau));
	} else if (sim.distType == DICE) {
		result = (lenA >= ceil(tau / (2-tau) * lenB)) &&
				 (lenA <= floor((2-tau) / tau * lenB));
	} else if (sim.distType == OLP) {
		result = (lenA >= tau) && (lenB >= tau);
	} else {
		print_debug("Unkown DIST_TYPE in LengthFilter\n");
	}
	//cout << "LengthFilter " << a.str.length() << "---------------" << b.str.length() << " " << result << " " << sim.dist << endl;
	return result;
}

string ContentFilter::Type() {
	return "ContentFilter";
}
bool ContentFilter::filter(const Field &a, const Field &b, const Similarity &sim) {
	bool result = false;
	if (sim.distType == ED || sim.distType == ES) {
		int ed = sim.distType == ED? sim.dist : ceil((1 - sim.dist) * max(a.str.length(), b.str.length()));
		int positive = 0;
		int negative = 0;
		result = true;
		for (auto apair : a.contentPairs_) {
			char c = apair.first;
			auto bpair = b.contentPairs_.find(c);
			int numb = (bpair == b.contentPairs_.end())? 0 : bpair->second;
			int delta = apair.second - numb;
			if (delta > 0) {
				positive += delta;
				if (positive > ed) {
					result = false;
					break;
				}
			}
			else {
				negative += -delta;
				if (negative > ed) {
					result = false;
					break;
				}
			}
		}
	} else
	// Jaccard has no ContentFilter
	{
		result = true;
	}
	//print_debug("use ContentFilter %s %s result = %d\n",a.str.c_str(), b.str.c_str(), result);
	return result;
}

vector<Filter*> g_filters;
void initFilters() {
	g_filters.clear();
	registerFilter(new LengthFilter());
	registerFilter(new Verifier());
	registerFilter(new ContentFilter());
}
void registerFilter(Filter *filter) {
	g_filters.push_back(filter);
}


