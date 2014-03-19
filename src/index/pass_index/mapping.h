#ifndef LIB_MAPPING_HPP
#define LIB_MAPPING_HPP

#include "common.h"

template <typename IDType>
class Mapping
{
public:
	typedef struct
	{
		IDType id;
		const char *content;
	} Value;

	Mapping()
	{
		map = NULL;
	}

	~Mapping()
	{
		if (map) free1d(map);
	}

	void init(int maxID)
	{
		alloc1d(map, maxID);
	}

	void clear()
	{
		size = 0;
	}

	int insert(const IDType id, const char *content)
	{
		map[size].id = id;
		map[size].content = content;
		return size++;
	}

	Value& get(int key)
	{
		return map[key];
	}
private:
	Value *map;
	int size = 0;
};

#endif

