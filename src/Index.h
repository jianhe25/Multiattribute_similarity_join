struct IndexItem {
	int id, pos;
	IndexItem(int tokensId, int _pos) : id(tokensId), pos(_pos) {}
};
typedef unordered_map<vector<IndexItem>> InvIndex;
typedef vector<InvIndex> LengthInvIndex;
vector<LengthInvIndex> tableInvIndex;

void buildPrefixIndex(const vector<string> &columns, Similarity &similarity);

int hashCode(const string &word);

void generateTokens(const vector<string> &column, Similarity &similarity,
	vector<vector<int>> &tokenContainer);

void sortByIDF(vector<vector<string>> &tokenContainer);

void buildInvertedIndex();


