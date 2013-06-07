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

Field::Field() {
	//cout << "Construct empty Field " << endl;
}
Field::Field(const string &_str, int _id) : str(_str), id(_id) {
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
void Field::GenerateGrams() {
	grams.clear();
	//cout << "str = " << str << " " << str.length() << " " << grams.size() << endl;
	for (int i = 0; i <= (int)str.length() - GRAM_LENGTH; ++i) {
		int t = HashCode(str.substr(i, GRAM_LENGTH));
		grams.push_back(t);
	}
	//cout << "Finish GenerateGrams " << endl;
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
