#include "tree_index.h"
#include <cassert>

#define PREFIX_BUCKET_SIZE 100

int Node::g_node_id = 0;
Node::Node(int max_length) {
	prefixMaps.resize(max_length + 1);
	id = ++g_node_id;
	isLeaf = false;
	leafIds.clear();
}

TreeIndex::TreeIndex(int a) {
	sims_.clear();
}
vector<pair<RowID, RowID>> TreeIndex::Join(Table &table1,
										   Table &table2,
										   vector<Similarity> &sims) {
	sims_ = sims;

	tablePtr1_ = &table1;
	tablePtr2_ = &table2;

	vector<int> row_ids1(table1.size(), 0);
	for (unsigned id = 0; id < table1.size(); ++id)
		row_ids1[id] = id;

	double time = getTimeStamp();
	root_ = new Node(g_string_max_length);

	//freopen("result", "w", stdout);
	BuildIndex(root_,
			sims,
			row_ids1,
			/*depth=*/0);
	printf("BuildIndex time: %.3fs\n", getTimeStamp() - time);

	// TODO: use only one vector<pair<RowID,RowID>> to hold all sim_pairs
	vector<pair<RowID,RowID> >  all_sim_pairs;
	for (int i = 0; i < int(table2.size()); ++i) {
			vector<pair<RowID,RowID> > temp = std::move(this->Search(table2[i]));
			for (const pair<int,int> &sim_pair : temp)
				all_sim_pairs.push_back(sim_pair);
			assert(!temp.empty());
			//cout << i << " " << debug_count_leaf_ << " " << candidates_.size() << endl;
	}
	return all_sim_pairs;
}

void TreeIndex::BuildIndex(Node *node,
		const vector<Similarity> &sims,
		const vector<int> &ids1,
		int depth) {

	if (depth == (int)sims.size() || depth == 3) {
		node->isLeaf = true;
		node->leafIds = vector<int>(ids1.begin(), ids1.end());
		//cout << node->id << " #leaf " << node->leafIds.size() << endl;
		return;
	}

	//for (int i = 0; i < depth; ++i) cout << '\t';
	//cout << node->id << " " << node << endl;

	Similarity sim = sims[depth];

	// TODO: add estimation logic here

    vector<int> lengthCounter(g_string_max_length + 1, 0);
    vector<int> hitCounter(g_string_max_length + 1, 0);
	vector<unordered_map<int, vector<int>>> id_set_per_length;
	id_set_per_length.resize(g_string_max_length + 1);
	for (int id : ids1) {
		const vector<int> &tokens = (*tablePtr1_)[id][sim.colx].tokens;
		int prefix_length = CalcPrefixLength(tokens.size(), sim);
		for (int i = 0; i < prefix_length; ++i) {
			int bucket_id = tokens[i] % PREFIX_BUCKET_SIZE;
			vector<int> &id_set = id_set_per_length[tokens.size()][bucket_id];
			if (id_set.empty() || id_set.back() != id) {
				id_set.push_back(id);
                hitCounter[tokens.size()] += 1;
            }
		}
        lengthCounter[tokens.size()] += prefix_length;
	}
    if (depth == 0) {
        for (int len = 0; len < (int)node->prefixMaps.size(); ++len)
            if (!id_set_per_length[len].empty()) {
                cout << "======================== len = " << len << " count = " << lengthCounter[len]
                     << " hit = " << hitCounter[len] << " prefix_len = " << CalcPrefixLength(len, sim) << " ==============" << endl;
                //for (const auto& kv : id_set_per_length[len]) {
                //cout << kv.first << ", id_set_size = " << kv.second.size() << endl;
                //}
            }
    }

	for (int len = 0; len < (int)node->prefixMaps.size(); ++len)
	if (!id_set_per_length[len].empty()) {
		//cout << "len = " << len << endl;
		auto &prefix_map = node->prefixMaps[len];
		for (const auto& kv : id_set_per_length[len]) {
			prefix_map[kv.first] = new Node(g_string_max_length);
			BuildIndex(prefix_map[kv.first],
                       sims,
                       kv.second,
                       depth+1);
		}
	}
}

// TODO: Call CalcOverlap many times
bool TreeIndex::VerifyRow(Row a, Row b) {
	for (const auto &sim : sims_)
		if (!verifier_.filter(a[sim.colx], b[sim.coly], sim)) {
			return false;
		}
	return true;
}
vector<pair<RowID, RowID> > TreeIndex::Search(const Row &row) {
	// Would change vector elements
	candidates_.clear();
	simPairs_.clear();
	debug_count_leaf_ = 0;
	this->TreeSearch(root_, row, 0);
	return simPairs_;
}
void TreeIndex::TreeSearch(Node *node, const Row &row, int depth) {
	if (depth == int(sims_.size()) || depth == 3) {
		debug_count_leaf_ += node->leafIds.size();
		for (int id : node->leafIds)
		if (candidates_.find(id) == candidates_.end()) {
			candidates_.insert(id);
			if (this->VerifyRow((*tablePtr1_)[id], row)) {
				simPairs_.push_back(make_pair(id, row[0].id));
			}
		}
		return;
	}

	vector<int> tokens = row[sims_[depth].coly].tokens;
	int len = tokens.size();
	pair<int,int> bound = CalcLengthBound(len, sims_[depth]);
	for (int l = bound.first; l <= min(int(node->prefixMaps.size())-1, bound.second); ++l)
	if (!node->prefixMaps[l].empty()) {
		auto &prefix_map = node->prefixMaps[l];
		if (prefix_map.empty()) continue;

		int prefix_length = CalcPrefixLength(len, sims_[depth]);
		vector<bool> visited(PREFIX_BUCKET_SIZE, false);
		for (int i = 0; i < prefix_length; ++i) {
			int bucket_id = tokens[i] % PREFIX_BUCKET_SIZE;
			if (prefix_map.find(bucket_id) == prefix_map.end())
				continue;
			if (visited[bucket_id])
				continue;
            visited[bucket_id] = true;
			Node *child = prefix_map[bucket_id];
			this->TreeSearch(child, row, depth + 1);
		}
	}
}

void TreeIndex::ShowIndex(Node *root) {
	std::queue<Node*> node_queue;
	node_queue.push(root);

	vector<int> leaf_size_count(2000, 0);
	while (!node_queue.empty()) {
		Node* node = node_queue.front();
		node_queue.pop();
		for (unsigned len = 0; len < node->prefixMaps.size(); ++len)
		if (!node->prefixMaps[len].empty()) {
			const auto &prefixMap = node->prefixMaps[len];
			for (const auto &kv : prefixMap) {
				node_queue.push(kv.second);
			}
		}
		if (node->isLeaf) {
			if (node->leafIds.size() < leaf_size_count.size())
				leaf_size_count[node->leafIds.size()]++;
		}
	}
}

