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
	vector<int> tokens;
	Field() {}
	Field(const string &_str);
};

typedef vector<Field> Row;
typedef vector<Field> Column;
typedef vector<Row> Table;

typedef enum {
	ED, JACCARD, NON_DEFINE
} DIST_TYPE;

struct Similarity {
	int colX, colY;
	double dist;
	DIST_TYPE distType;

	Similarity(DIST_TYPE _distType, double _dist) :
		colX(0), colY(0), dist(_dist), distType(_distType) {}

	Similarity(int _colX, int _colY, double _dist, DIST_TYPE _distType) :
		colX(_colX), colY(_colY), dist(_dist), distType(_distType) {}
	Similarity() {}
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

#endif // SRC_CORE_H

