#include "core.h"
#include <algorithm>
#include <cassert>
using namespace std;

unordered_map<string, int> g_string_map;
int g_string_max_length;

int HashCode(const string &word) {
	if (g_string_map.find(word) == g_string_map.end()) {
		g_string_map.insert(make_pair(word, g_string_map.size()));
	}
	return g_string_map[word];
}

Field::Field() {
	//cout << "Construct empty Field " << endl;
}
Field::Field(const string &_str, int _id) : str(_str), id(_id) {
}

void Field::GenerateTokens() {
	vector<string> words;
	splitString(str.c_str(), ' ', words);
	tokens.clear();
	for (auto word : words) {
		stripString(word);
		tokens.push_back(HashCode(word));
	}
	// TODO: to sort by TF in the future
	sort(tokens.begin(), tokens.end());
    g_string_max_length = max(g_string_max_length, (int)tokens.size());
}
void Field::GenerateGrams() {
	tokens.clear();
	for (int i = 0; i <= int(str.length() - GRAM_LENGTH); ++i) {
		int t = HashCode(str.substr(i, GRAM_LENGTH));
		tokens.push_back(t);
	}
	if (str.length() < GRAM_LENGTH)
		tokens.push_back(HashCode(str));
	// TODO: to sort by TF in the future
	sort(tokens.begin(), tokens.end());
    g_string_max_length = max(g_string_max_length, (int)tokens.size());
}
Field::~Field() {
	//cout << "deconstruct field  " << str << endl;
}


int printCounter = 0;
void print(const Table &table) {
	cout << "================================== " << ++printCounter << "\n";
	for (const Row &row : table) {
		for (const Field &field : row) {
			//cout << field.id << endl;
			for (const auto token : field.tokens)
				cout << token << " ";
			cout << endl;
		}
	}
	cout << "==================================\n";
}

int CalcOverlap(int lenS, int lenR, const Similarity &sim) {
	//cout << "CalcOverlap " << lenS << " " << lenR << " " << int(ceil((lenS + lenR) * sim.dist / (1.0+sim.dist)));
	if (sim.distType == ED)
		return max(lenS, lenR) - GRAM_LENGTH * int(sim.dist);
	else if (sim.distType == JACCARD)
		return int(ceil((lenS + lenR) * sim.dist / (1.0 + sim.dist)));
	else {
		cerr << "Unkown DIST_TYPE in CalcOverlap" << endl;
		return -1;
	}
}
int CalcPrefixLength(int size, const Similarity& sim) {
	double tau = sim.dist;
	int common = -1;
	if (sim.distType == JACCARD)
		common = size * tau / (1.0 + tau);
	else if (sim.distType == COSINE)
		common = sqrt(double(size)) * tau;
	else if (sim.distType == DICE)
		common = size * tau / 2.0;
	else if (sim.distType == ED)
		common = max(0, size - (int)(tau * GRAM_LENGTH));
	else {
        print_debug("Unkown DIST_TYPE in CalcPrefixLength %d\n", sim.distType);
		assert(0);
		return -1;
	}
	return size - max(common-1, 0);
}

// TODO: Precompute Bound
pair<int,int> CalcLengthBound(int lenS, const Similarity &sim) {
	if (sim.distType == ED) {
		return make_pair(max(0, int(lenS - sim.dist)), int(lenS + sim.dist));
	} else if (sim.distType == JACCARD) {
		double c = sim.dist / (1.0 + sim.dist);
		return make_pair(max(0, int(lenS * c / (1.0 - c))), int(lenS / c - lenS));
	}
	cerr << "Unkown DIST_TYPE in CalcLengthBound" << endl;
	return make_pair(-1, -1);
}

void printRow(const vector<Field> &row) {
    for (auto field : row)
        cout << field.str << " ";
    cout << endl;
}

