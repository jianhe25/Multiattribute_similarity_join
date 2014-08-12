#include "core.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
using namespace std;

unordered_map<string, int> g_string_map;
int g_string_max_length;
unordered_map<int,int> g_token_counter[MAX_COLUMN_NUM]; // MAX COLUMN NUMBER = 10
unordered_map<int,int> g_id_map[MAX_COLUMN_NUM]; // MAX COLUMN NUMBER = 10

int HashCode(const string &word) {
	if (g_string_map.find(word) == g_string_map.end()) {
		g_string_map.insert(make_pair(word, g_string_map.size()));
	}
	return g_string_map[word];
}

Field::Field() {
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
	//sort(tokens.begin(), tokens.end());
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
	//sort(tokens.begin(), tokens.end());
    g_string_max_length = max(g_string_max_length, (int)tokens.size());
}

//void Field::GeneratePositionGrams() {
	//positionedTokens.clear();
	//for (int i = 0; i <= int(str.length() - GRAM_LENGTH); ++i) {
		//int t = HashCode(str.substr(i, GRAM_LENGTH));
		//positionedTokens.push_back(make_pair(t, i));
	//}
	//if (str.length() < GRAM_LENGTH)
		//positionedTokens.push_back(HashCode(str));
	//// TODO: to sort by TF in the future
	//sort(positionedTokens.begin(), positionedTokens.end());
    //g_string_max_length = max(g_string_max_length, (int)tokens.size());
//}

void Field::GenerateContent() {
	contentPairs_.clear();
	for (int i = 0; i < int(str.length()); ++i) {
		contentPairs_[str[i]]++;
	}
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

	double tau = sim.dist;
	if (sim.distType == ED)
		return max(0, max(lenS, lenR) + 1 - GRAM_LENGTH * int(tau + 1));
	else if (sim.distType == JACCARD)
		return int(ceil((lenS + lenR) * tau / (1.0 + tau)));
	else if (sim.distType == COSINE)
		return int(ceil(sqrt(lenS * lenR) * tau));
	else if (sim.distType == DICE)
		return int(ceil((lenS + lenR) * tau  / 2.0));
	else if (sim.distType == OLP)
		return int(ceil(tau));
	else if (sim.distType == ES) {
		int ed = ceil((1-tau) * max(lenR, lenS));
		return max(0, max(lenS, lenR) + 1 - GRAM_LENGTH * int(ed + 1));
	} else {
		print_debug("Error: Unkown DIST_TYPE in CalcOverlap");
		return -1;
	}
}

int numCount = 0;
int CalcPrefixLength(int lenR, const Similarity& sim) {
	double tau = sim.dist;
	int common = -1;
	/*
	 * Overlap deduced in paper
	 */
	if (sim.distType == JACCARD)
		common = ceil(lenR * tau);
	else if (sim.distType == COSINE)
		common = ceil(lenR * tau * tau);
	else if (sim.distType == DICE)
		common = ceil(tau * lenR / (2.0 - tau));
	else if (sim.distType == OLP)
		common = ceil(tau);
	else if (sim.distType == ED)
		common = max(0, lenR + 1 - GRAM_LENGTH - (int)(tau * GRAM_LENGTH));
	else if (sim.distType == ES)
		common = max(0, int(ceil(lenR + 1 - ((1-tau) * lenR + 1) * GRAM_LENGTH)));
	else {
		print_debug("Error: Unkown DIST_TYPE in CalcPrefixLength ");
		cout << sim.type() << endl;
		assert(0);
		return -1;
	}

	//if (sim.distType == JACCARD && ++numCount % 10000 == 0) {
	//print_debug("common: %d prefix_length: %d\n", common, lenR - max(common-1, 0));
	//}
	return max(0, lenR - max(common-1, 0));

	/*
	 * Overlap deduced by myself
	 */
	//if (sim.distType == JACCARD)
		//common = lenR * tau / (1.0 + tau);
	//else if (sim.distType == COSINE)
		//common = sqrt(double(lenR)) * tau;
	//else if (sim.distType == DICE)
		//common = lenR * tau / 2.0;
	//else if (sim.distType == OLP)
		//common = tau;
	//else if (sim.distType == ED)
		//common = max(0, lenR - (int)(tau * GRAM_LENGTH));
	//else {
        //print_debug("Unkown DIST_TYPE in CalcPrefixLength %d\n", sim.distType);
		//assert(0);
		//return -1;
	//}
}

void printRow(const vector<Field> &row) {
    for (auto field : row)
        cout << field.str << "| ";
    cout << endl;
}
void GenerateContent(const vector<Similarity> &sims, Table &table, int isColy) {
	// Generate Content
	for (const auto &sim : sims) {
		int col = isColy? sim.coly : sim.colx;
		for (unsigned i = 0; i < table.size(); ++i) {
            if (sim.distType == ED || sim.distType == ES) {
				table[i][col].GenerateContent();
			}
        }
    }
}

bool CompareTokenByTF(const pair<int,int> &a, const pair<int,int> &b) {
	return a.second < b.second;
}
void GenerateTokensOrGram(const vector<Similarity> &sims, Table &table, int isColy) {
	// GenerateTokens or GenerateGrams
	for (const auto &sim : sims) {
		int col = isColy? sim.coly : sim.colx;
		for (unsigned i = 0; i < table.size(); ++i) {
            if (sim.distType == JACCARD) {
				table[i][col].GenerateTokens();
			} else {
				table[i][col].GenerateGrams();
			}
        }
		unordered_map<int,int> &token_counter = g_token_counter[col];
		unordered_map<int,int> &id_map = g_id_map[col];
		if (!isColy) {
			for (unsigned i = 0; i < table.size(); ++i) {
				const vector<int> &tokens = table[i][col].tokens;
				for (int token : tokens)
					token_counter[token]++;
				//sort(tokens.begin(), tokens.end());
			}

			vector<pair<int,int>> packs;
			for (auto kv : token_counter)
				packs.push_back(make_pair(kv.first, kv.second));
			sort(packs.begin(), packs.end(), CompareTokenByTF);

			for (int i = 0; i < packs.size(); ++i)
				id_map.insert(make_pair(packs[i].first, i));
		}
		for (unsigned i = 0; i < table.size(); ++i) {
			vector<int> &tokens = table[i][col].tokens;
			for (int i = 0; i < tokens.size(); ++i) {
				const auto it = id_map.find( tokens[i] );
				if (it == id_map.end())
					tokens[i] = -1;
				else
					tokens[i] = it->second;
			}
			sort(tokens.begin(), tokens.end());
		}
	}
}

DIST_TYPE getSimType(const string &operand) {
	if (operand == "ED")
		return ED;
	if (operand == "JACCARD")
		return JACCARD;
	if (operand == "COSINE")
		return COSINE;
	if (operand == "DICE")
		return DICE;
	if (operand == "OLP")
		return OLP;
	if (operand == "ES")
		return ES;
	return NON_DEFINE;
}

void PrintSims(const vector<Similarity> &sims) {
	puts("==================================");
	puts("Mapping rules:");
	for (auto sim : sims) {
		sim.echo();
	}
	puts("==================================");
}

void ExportTime(string message, double time) {
	static ofstream stat_file;
	static ofstream formated_stat_file;
	if (!stat_file.is_open())
		stat_file.open("time_stat", ios::out | ios::app);
	if (!formated_stat_file.is_open())
		formated_stat_file.open("formated_stat_file", ios::out | ios::app);
	if (message == "numCandidatePairs") {
		stat_file << message << ": " << (long long)time << " ";
		formated_stat_file << (long long)time << " ";
	} else {
		stat_file << message << ": " << time << " ";
		formated_stat_file << time << " ";
	}
	if (message == "total")
		stat_file << "\n";
	stat_file.close();
}

