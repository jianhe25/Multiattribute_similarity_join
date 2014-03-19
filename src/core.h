#pragma once

#include "common.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include <math.h>
using namespace std;

#define GRAM_LENGTH 3
#define SAMPLE_RATIO 0.05

typedef int RowID;
extern unordered_map<string, int> g_string_map;
extern int g_string_max_length;

int HashCode(const string &word);

struct Field {
	string str;
	int id;
	vector<int> tokens;
	Field();
    ~Field();
	Field(const string &_str, int id=0);
	void GenerateGrams();
    void GenerateTokens();
};

typedef vector<Field> Row;
typedef vector<Field> Column;
typedef vector<Row> Table;

typedef enum {
	ED, JACCARD, COSINE, DICE, NON_DEFINE
} DIST_TYPE;

struct Similarity {
	int colx, coly;
	double dist;
	DIST_TYPE distType;
	bool isSearched;

	Similarity(DIST_TYPE _distType, double _dist) :
		colx(0), coly(0), dist(_dist), distType(_distType), isSearched(false) {}

	Similarity(int _colx, int _coly, double _dist, DIST_TYPE _distType) :
		colx(_colx), coly(_coly), dist(_dist), distType(_distType), isSearched(false) {}

	Similarity() : isSearched(false) {}

};

void print(const Table &table);

int CalcOverlap(int lenS, int lenR, const Similarity &sim);

int CalcPrefixLength(int len, Similarity sim);

pair<int,int> CalcLengthBound(int lenS, const Similarity &sim);
