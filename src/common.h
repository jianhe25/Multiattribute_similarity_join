#ifndef SRC_COMMON_H
#define SRC_COMMON_H

#include <vector>
#include <string>
using namespace std;

void splitString(const char *s, char delimiter, vector<string> &container);
void stripString(string &word);
int getTimeStamp();

/*
 * tranpose row table to column table
 */
template<class T>
void transpose(const vector<vector<T>> &table, vector<vector<T>> *column_table) {
	int column_num = table[0].size();
	column_table->resize(column_num, vector<T>(table.size(), T()));
	for (int i = 0; i < (int)table.size(); ++i)
		for (int j = 0; j < column_num; ++j)
			(*column_table)[j][i] = table[i][j];
}
#endif // SRC_COMMON_H

