#include "common.h"
#include <sys/time.h>

/*
 * Split String by delimiter and strip trailing blanks
 */
void splitString(const char *s, char delimiter, vector<string> &container)
{
	string str = s;
	container.clear();
	int start = 0;
	while (true)
	{
		auto pos = str.find(delimiter, start);
		if (pos != string::npos)
		{
			container.push_back(str.substr(start, pos - start));
			start = pos + 1;
		} else {
			container.push_back(str.substr(start));
			return;
		}
	}
}

void stripString(string &word) {
	while (word.back() == ' ' || word.back() == '\t' || word.back() == '\n' || word.back() == '\r')
		word.pop_back();
}

int getTimeStamp()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000 + t.tv_usec;
}

