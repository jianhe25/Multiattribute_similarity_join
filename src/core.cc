#include "core.h"
#include <algorithm>
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

int CalcPrefixLength(int len, Similarity sim) {
	if (sim.distType == ED) {
		//cout << "prefixLength = " << min(len, GRAM_LENGTH * int(sim.dist) + 1) << endl;
		return min(len, GRAM_LENGTH * int(sim.dist) + 1);
	} else if (sim.distType == JACCARD) {
		return min(len, len - int(ceil(len * sim.dist / (1.0 + sim.dist))) + 1);
	}
	cerr << "Unkown DIST_TYPE in CalcPrefixLength" << endl;
	return -1;
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

