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
}

void Field::GeneratePositionTokens() {
	vector<string> words;
	splitString(str.c_str(), ' ', words);
	positionedTokens.clear();
	for (int i = 0; i < words.size(); ++i) {
		stripString(words[i]);
		positionedTokens.push_back(make_pair(HashCode(words[i]), i));
	}
}

void Field::GenerateGrams() {
	tokens.clear();
	for (int i = 0; i <= int(str.length() - GRAM_LENGTH); ++i) {
		int t = HashCode(str.substr(i, GRAM_LENGTH));
		tokens.push_back(t);
	}
	if (str.length() < GRAM_LENGTH)
		tokens.push_back(HashCode(str));
}

void Field::GeneratePositionGrams() {
	positionedTokens.clear();
	for (int i = 0; i <= int(str.length() - GRAM_LENGTH); ++i) {
		int t = HashCode(str.substr(i, GRAM_LENGTH));
		positionedTokens.push_back(make_pair(t, i));
	}
	if (str.length() < GRAM_LENGTH)
		positionedTokens.push_back(make_pair(HashCode(str), 0));
}

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

int edjoin_prefix_length(const vector<pair<int, int>> &positionedTokens, const Similarity &sim)
{
	int threshold = sim.distType == ED? sim.dist : ceil((1 - sim.dist) * (positionedTokens.size()+GRAM_LENGTH-1));
	int low = threshold + 1;
	int high = GRAM_LENGTH * threshold + 1;
	// binary search minimum gram length
	while (low < high)
	{
		int mid = (low + high) / 2;
		int errors = 0, location = 0;
		vector<pair<int, int>> duplicate(positionedTokens.begin(), positionedTokens.begin() + mid);
		sort(duplicate.begin(), duplicate.end(), [](const pair<int, int> &p1, const pair<int, int> &p2) {
				return p1.second < p2.second;
				});
		for (int k = 0; k < mid; k++)
		{
			if (duplicate[k].second >= location)
			{
				errors++;
				location = duplicate[k].second + GRAM_LENGTH;
			}
		}
		if (errors <= threshold)
		{
			low = mid + 1;
		} else {
			high = mid;
		}
	}
	return low;
}

int numCount = 0;
int CalcPrefixLength(const Field &field, const Similarity& sim) {
	int lenR = field.tokens.size();
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
	else if (sim.distType == ED) {
		if (FLAGS_baseline_exp == "edjoin") {
			return edjoin_prefix_length(field.positionedTokens, sim);
		}
		common = max(0, lenR + 1 - GRAM_LENGTH - (int)(tau * GRAM_LENGTH));
	}
	else if (sim.distType == ES) {
		common = max(0, int(ceil(lenR + 1 - ((1-tau) * lenR + 1) * GRAM_LENGTH)));
		if (FLAGS_baseline_exp == "edjoin") {
			//print_debug("fileld.positionedTokens = %d\n", field.positionedTokens.size());
			return edjoin_prefix_length(field.positionedTokens, sim);
		}
	}
	else {
		print_debug("Error: Unkown DIST_TYPE in CalcPrefixLength ");
		cout << sim.type() << endl;
		assert(0);
		return -1;
	}

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

void GeneratePositionTokenOrGram(const vector<Similarity> &sims, Table &table, int isColy) {
	// GenerateTokens or GenerateGrams
	for (const auto &sim : sims) {
		int col = isColy? sim.coly : sim.colx;
		for (unsigned i = 0; i < table.size(); ++i) {
            if (sim.distType == JACCARD) {
				table[i][col].GeneratePositionTokens();
			} else {
				table[i][col].GeneratePositionGrams();
			}
        }
		unordered_map<int,int> &token_counter = g_token_counter[col];
		unordered_map<int,int> &id_map = g_id_map[col];
		if (!isColy) {
			for (unsigned i = 0; i < table.size(); ++i) {
				const vector<pair<int,int>> &positionedTokens = table[i][col].positionedTokens;
				for (pair<int,int> token_pos : positionedTokens)
					token_counter[token_pos.first]++;
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
			vector<pair<int,int>> &positionedTokens = table[i][col].positionedTokens;
			for (int i = 0; i < positionedTokens.size(); ++i) {
				const auto it = id_map.find( positionedTokens[i].first );
				if (it == id_map.end())
					positionedTokens[i].first = -1;
				else
					positionedTokens[i].first = it->second;
			}
			sort(positionedTokens.begin(), positionedTokens.end());
			for (const pair<int,int> &token_pos : positionedTokens)
				table[i][col].tokens.push_back(token_pos.first);
		}
	}
}

void GenerateTokensOrGram(const vector<Similarity> &sims, Table &table, int isColy) {
	if (FLAGS_baseline_exp == "edjoin") {
		GeneratePositionTokenOrGram(sims, table, isColy);
		return;
	}
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

pair<int,int> ComputeLengthBound(const Field &query, const Similarity &sim) {
	if (sim.distType == ED || sim.distType == ES) {
		int ed = sim.distType == ED? sim.dist : ceil((1 - sim.dist) * query.str.length());
		return make_pair(query.tokens.size() - ed, query.tokens.size() + ed);
	}
	// default don't know
	return make_pair(-1, -1);
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

