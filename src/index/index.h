#ifndef SRC_INDEX_INDEX_H
#define SRC_INDEX_INDEX_H

#include "../common.h"
#include "../core.h"
#include "../filter.h"
#include <vector>
#include <gflags/gflags.h>
using namespace std;

DECLARE_int32(index_version);
class SimIndex {
public:
	Similarity *sim_;
	vector<Field> *fields_;

	virtual void build(vector<Field> &fields, Similarity *sim) = 0;

	virtual void search(Field &query, vector<int> *matchIDs) = 0;

	virtual SimIndex *GetInstance() = 0;
};
class SimIndexFactory {
public:
	static SimIndex *g_index;

	static void InstallIndex();

	static SimIndex *GetIndex();
};

class EmptyIndex : public SimIndex {

	void build(vector<Field> &fields, Similarity *sim);

	void search(Field &query, vector<int> *matchIDs);

	SimIndex* GetInstance();
};
#endif // SRC_INDEX_INDEX_H
