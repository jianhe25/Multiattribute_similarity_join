#ifndef SRC_COMMON_H
#define SRC_COMMON_H

#include <vector>
#include <string>
#include <iostream>
using namespace std;

void splitString(const char *s, char delimiter, vector<string> &container);
void stripString(string &word);
int getTimeStamp();
void PrintTime(int milli_sec);

/*
 * tranpose row table to column table
 */
template<class T>
void transpose(const vector<vector<T>> &table, vector<vector<T>> *column_table) {
	int column_num = table[0].size();
	column_table->resize(column_num);
	for (int i = 0; i < (int)table.size(); ++i)
		for (int j = 0; j < column_num; ++j)
			(*column_table)[j].push_back(table[i][j]);
}

#endif // SRC_COMMON_H

