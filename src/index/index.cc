#include "index.h"
#include "prefix_index/prefix_index.h" // prefix_index is default index
#include <iostream>
using namespace std;

DEFINE_int32(index_version, 0, "index version, 0 means no index at all");

SimIndex* SimIndexFactory::g_index = NULL;
SimIndex* SimIndexFactory::GetIndex() {
	return g_index;
}
void SimIndexFactory::InstallIndex() {
	switch(FLAGS_index_version) {
		case 0 : { g_index = new EmptyIndex(); break; }
		case 1 : { g_index = new PrefixIndex(); break; }
		default : {
			cerr << "index version is out of range" << endl;
			break;
		}
	}
}
void EmptyIndex::build(vector<Field> &fields, Similarity *sim) {
	sim_ = sim;
	fields_ = &fields;
}
void EmptyIndex::search(Field &query, vector<int> *matchIDs) {
	for (int i = 0; i < (int)fields_->size(); ++i)
		matchIDs->push_back(i);
}
SimIndex* EmptyIndex::GetInstance() {
	return new EmptyIndex();
}
