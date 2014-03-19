#pragma once

#include <vector>
#include <string>
/*#include <iostream>*/
using namespace std;

void splitString(const char *s, char delimiter, vector<string> &container);
void stripString(string &word);
double getTimeStamp();
void PrintTime(int milli_sec);

/*
 * tranpose row table to column table
 */
template<class T>
void transpose(const vector<vector<T>> &table, vector<vector<T>> *column_table) {
    int num_row = table.size();
	int num_col = table[0].size();
	column_table->resize(num_col);
	for (int j = 0; j < num_col; ++j)
        (*column_table)[j].resize(num_row);
	for (int i = 0; i < (int)table.size(); ++i) {
		for (int j = 0; j < num_col; ++j)
			(*column_table)[j][i] = table[i][j];
    }
}

