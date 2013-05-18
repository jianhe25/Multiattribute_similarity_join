#include "core.h"
#include <algorithm>
using namespace std;

unordered_map<string, int> g_string_map;

int HashCode(const string &word) {
	if (g_string_map.find(word) == g_string_map.end()) {
		g_string_map.insert(make_pair(word, g_string_map.size()));
	}
	return g_string_map[word];
}

Field::Field(const string &_str) : str(_str) {
	vector<string> words;
	splitString(str.c_str(), ' ', words);
	tokens.clear();
	for (auto word : words) {
		stripString(word);
		tokens.push_back(HashCode(word));
	}
	// TODO: to sort by index in the future
	sort(tokens.begin(), tokens.end());
}


