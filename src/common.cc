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

void PrintTime(int milli_sec)
{
	milli_sec /= 1000;
	int v=milli_sec;
	int hours=v/(1000*60*60); v%=(1000*60*60);
	int minutes=v/(1000*60); v%=(1000*60);
	int seconds=v/1000; v%=1000;
	int milli_seconds=v;
	int first=1;
	printf("%d[", milli_sec);
	if(hours) {if(!first) printf(":"); printf("%dh", hours); first=0;}
	if(minutes) {if(!first) printf(":"); printf("%dm", minutes); first=0;}
	if(seconds) {if(!first) printf(":"); printf("%ds", seconds); first=0;}
	if(milli_seconds) {if(!first) printf(":"); printf("%dms", milli_seconds); first=0;}
	printf("]");
}

