#ifndef SRC_CORE_H
#define SRC_CORE_H

#include "common.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include <math.h>
using namespace std;

#define GRAM_LENGTH 4
#define SAMPLE_RATIO 0.05

extern unordered_map<string, int> g_string_map;

int HashCode(const string &word);

struct Field {
	string str;
	int id;
	vector<int> tokens;
	vector<int> grams;
	Field();
	~Field();
	Field(const string &_str, int id=0);
	void GenerateGrams();
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

	int CalcOverlap(int lenS, int lenR) const {
		if (distType == ED)
			return max(lenS, lenR) - GRAM_LENGTH * int(dist);
		else if (distType == JACCARD)
			return int(ceil((lenS + lenR) * dist / (1+dist)));
		else {
			cerr << "Unkown DIST_TYPE in CalcOverlap" << endl;
			return -1;
		}
	}
};

void print(const Table &table);
#endif // SRC_CORE_H

