#ifndef LIB_HASH_H
#define LIB_HASH_H

#include "common.h"

class Hash
{
public:
	static void init()
	{
		alloc1d(power, SAFELEN);

		power[0] = 1;
		for (int i = 1; i < SAFELEN; i++)
			power[i] = power[i - 1] * BASE;
	}

	static void destroy()
	{
		free1d(power);
	}

	static unsigned long long getHash(const char *str, int pos, int len)
	{
		unsigned long long result = 0;
		for (int i = 0; i < len; i++)
			result += power[len - i - 1] * (unsigned char)str[pos + i];
		return result;
	}

	void initQuery(const char *query, int len)
	{
		preHash[0] = 0;
		for (int i = 0; i < len; i++)
			preHash[i + 1] = preHash[i] * BASE + (unsigned char)query[i];
	}

	unsigned long long getQueryValue(int startPos, int endPos)
	{
		return preHash[endPos] - preHash[startPos] * power[endPos - startPos];
	}

	void destroyQuery()
	{
	}
private:
	unsigned long long preHash[SAFELEN];
	static const unsigned long long BASE = 131ULL;
	static unsigned long long *power;
};

#endif

