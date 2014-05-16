#ifndef SRC_INDEX_INDEX_H
#define SRC_INDEX_INDEX_H

#include "../common.h"
#include "../core.h"
#include "../filter.h"
#include <vector>
#include <gflags/gflags.h>
#include <unordered_set>
using namespace std;

DECLARE_int32(index_version);
extern const int PREFIX_SINGLE_INDEX;
extern const int PREFIX_TREE_INDEX;

class SimIndex {
public:
	int indexType_;

	Similarity *sim_;

	vector<Field*> *fields_;

	virtual void build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim) = 0;

	virtual void search(Field &query, vector<int> *matchIDs) = 0;

	virtual SimIndex *GetInstance() = 0;

    virtual unordered_set<int> getPrefixList(Field &query);
	virtual int calcPrefixListSize(Field &query);
};
class SimIndexFactory {
public:
	static SimIndex *g_index;

	static void InstallIndex();

	static SimIndex *GetIndex();
};

class EmptyIndex : public SimIndex {

	void build(vector<Field*> &fields1, vector<Field*> &fields2, Similarity *sim);

	void search(Field &query, vector<int> *matchIDs);

	SimIndex* GetInstance();
};
#endif // SRC_INDEX_INDEX_H
