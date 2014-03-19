#ifndef LIB_INDEX_H
#define LIB_INDEX_H

#include "common.h"

class Part
{
public:
	int stPos;    // start position of string
	int Lo;       // start position of segment
	int partLen;  // substring/segment length
	int len;      // length of indexed string
	Part(int _s, int _o, int _p, int _l)
		: stPos(_s), Lo(_o), partLen(_p), len(_l) {}
};

class passIndex
{
	template<typename IDType> friend class Search;
public:
	void init(int);
	void insert(int, const char*);
	void clear();
	Index();
	~Index();
private:
	void insertInternal(int, const char*);
	void insert0Internal(int, const char*);

	std::tr1::unordered_map<unsigned long long, std::vector<int>> **invList;
	std::tr1::unordered_map<std::string, int> index0;
	std::vector<Part> **partIndex;
	int **partLen, **partPos;
	int D, PN;
};

#endif

