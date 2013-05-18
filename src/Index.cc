#include "Index.h"
#include "common.h"
#include <unordered_map>
#include <unordered_set>
using namespace std;

unordered_map<string> wordMap;
int hashCode(const string &word) {
	if (wordMap.find(word) == wordMap.end())
		wordMap.insert( make_pair(word, wordMap.size()) );
	return wordMap[word];
}


void generateTokens(const vector<string> &column,
		DistType distType,
		TokenColumn *tokenColumn) {
	for (auto str : column) {
		vector<int> tokens;
		if (distType == ED) {
			for (unsigned i = 0; i <= str.size() - GRAM_LENGTH; ++i) {
				string word = str.substr(i, GRAM_LENGTH);
				tokens->push_back(hashCode(word));
			}
		} else {
			splitString(str, ' ', &tokens);
		}
		tokenColumn->push_back(tokens);
	}
}

int calcOverlap(const Similarity &similarity, vector<int> tokens) {
	if (similarity.distType == ED)
		return tokens.size() - (int)similarity.dist * GRAM_LENGTH;
	if (similarity.distType == JACCARD)
		return tokens.size() * similarity.dist / (1 + similarity.dist);
}

void buildInvertedIndex(const TokenColumn &tokenColumn,
						const Similarity &similarity,
						LengthInvIndex *invIndices) {
	fieldId = 0;
	for (auto field : tokenColumn) {
		int pos = 0;
		for (auto token : field)
			invIndices[len][token]->push_back(IndexItem(fieldId, pos++));
		fieldId++;
	}
}


