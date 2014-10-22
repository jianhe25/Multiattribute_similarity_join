#pragma once

#include "common.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include <math.h>
#include <gflags/gflags.h>
using namespace std;

#define GRAM_LENGTH 3
#define SAMPLE_RATIO 0.005

DECLARE_string(baseline_exp);

const int MAX_COLUMN_NUM = 10;

typedef int RowID;
extern unordered_map<string, int> g_string_map;
extern int g_string_max_length;
extern unordered_map<int, int> g_token_counter[MAX_COLUMN_NUM];
extern unordered_map<int, int> g_id_map[MAX_COLUMN_NUM];

int HashCode(const string &word);

struct Field {
	string str;
	int id;
	vector<int> tokens;
	vector<pair<int, int>> positionedTokens;
	unordered_map<char, int> contentPairs_;
	int prefixLength;
	Field();
    ~Field();
	Field(const string &_str, int id=0);
	void GenerateGrams();
    void GenerateTokens();
	void GenerateContent();
	void GeneratePositionGrams();
	void GeneratePositionTokens();
};

typedef vector<Field> Row;
typedef vector<Field> Column;
typedef vector<Row> Table;

typedef enum {
	ED,
	JACCARD,
	COSINE,
	DICE,
	OLP,
	ES,
	NON_DEFINE
} DIST_TYPE;

struct Similarity {
	int colx, coly;
	double dist;
	DIST_TYPE distType;
	int num_estimated_candidates;
	long long splitedCost;
	double splitedEntropy;

	Similarity(DIST_TYPE _distType, double _dist) :
		colx(0), coly(0), dist(_dist), distType(_distType) {
		print_debug("Error: this constructor should never be called");
	}

	Similarity(int _colx, int _coly, double _dist, DIST_TYPE _distType) :
		colx(_colx), coly(_coly), dist(_dist), distType(_distType) {
	}

	Similarity() {
	}

	bool isSetSim() const {
		return distType == JACCARD || distType == COSINE || distType == DICE || distType == OLP;
	}

	string type() const {
		if (distType == ED)
			return "ED";
		else if (distType == JACCARD)
			return "JACCARD";
		else if (distType == COSINE)
			return "COSINE";
		else if (distType == DICE)
			return "DICE";
		else if (distType == OLP)
			return "OLP";
		else if (distType == ES)
			return "ES";
		else
			return "NON_DEFINE";
	}
	void echo() const {
		if (distType == ED)
			cout << type() << "(" << colx << ", " << coly << ") >= " << dist << endl;
		else
			cout << type() << "(" << colx << ", " << coly << ") <= " << dist << endl;
	}
};

struct Query {
	Row row;
	vector<Similarity> sims;
	int id;
};

void printTable(const Table &table);

int CalcOverlap(int lenS, int lenR, const Similarity &sim);

int CalcPrefixLength(Field &field, const Similarity &sim);

void printRow(const Row &row);

void GenerateContent(const vector<Similarity> &sims, Table &table, int isColy);

void GenerateTokensOrGram(const vector<Similarity> &sims, Table &table, int isColy);

void GeneratePositionTokenOrGram(const vector<Similarity> &sims, Table &table, int isColy);

DIST_TYPE getSimType(const string &operand);

void PrintSims(const vector<Similarity> &sims);

void ExportTime(string message, double time);

int edjoin_prefix_length(const vector<pair<int, int>> &positionedTokens, const Similarity &sim);

pair<int,int> ComputeLengthBound(const Field &query, const Similarity &sim);
