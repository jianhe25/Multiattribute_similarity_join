#ifndef LIB_SEARCH_H
#define LIB_SEARCH_H

#include "common.h"
#include "mapping.h"
#include "../Index.h"
#include "../../common.h"

class Search : SimIndex
{
public:
	void build(SharedInfo<IDType> *sharedInfo)
	{
		this->sharedInfo = sharedInfo;
		for (int k = 0; k < SAFELEN; k++)
		{
			matrix[k][0] = _mm_set1_epi8(k);
			matrix[0][k] = _mm_set1_epi8(k);
		}
	}

	void insertED(int ed, IDType id, const char *record)
	{
		edIndexes[ed].insert(mapping.insert(id, record), record);
	}

	void insertHM(int hm, IDType id, const char *record)
	{
		hmIndexes[hm].insert(mapping.insert(id, record), record);
	}

	void search(const string &query, Similarity similarity, vector<int> *ids) {
	}

	void searchHM(int hm, const char *query, std::vector<IDType> &ids, std::vector<int> &hms)
	{
		D = hm;
		index = &(sharedInfo->hmIndexes[D]);

		if (D == 0)
		{
			auto iter = index->index0.find(query);
			if (iter != index->index0.end())
			{
				auto &value = sharedInfo->mapping.get(iter->second);
				ids.push_back(value.id);
				hms.push_back(0);
			}
			return;
		}

		IDType id[16];
		int result[16];
		int count = 0;

		int qlen = strlen(query);
		hash.initQuery(query, qlen);
		for (int partId = 0; partId < index->PN; partId++)
		{
			int len = index->partLen[partId][qlen];
			int pos = index->partPos[partId][qlen];
			auto value = hash.getQueryValue(pos, pos + len);
			auto inv_it = index->invList[partId][qlen].find(value);
			if (inv_it == index->invList[partId][qlen].end()) continue;

			// enumerate all element in inverted list
			for (auto vit = inv_it->second.begin(); vit != inv_it->second.end(); vit++)
			{
				auto &value = sharedInfo->mapping.get(*vit);
				const char *candidate = value.content;
				for (int k = 0; k < qlen; k++)
					buffer[k][count] = candidate[k];
				id[count] = value.id;

				if (++count == 16)
				{
					getHM((__m128i*)buffer, query, qlen, result, count, D);
					for (int k = 0; k < 16; k++)
					{
						if (result[k] <= D)
						{
							ids.push_back(id[k]);
							hms.push_back(result[k]);
						}
					}
					count = 0;
				}
			}
		}

		if (count != 0)
		{
			getHM((__m128i*)buffer, query, qlen, result, count, D);
			for (int k = 0; k < count; k++)
			{
				if (result[k] <= D)
				{
					ids.push_back(id[k]);
					hms.push_back(result[k]);
				}
			}
		}

		hash.destroyQuery();
	}

	void searchED(int ed, const char *query, std::vector<IDType> &ids, std::vector<int> &eds)
	{
		D = ed;
		index = &(sharedInfo->edIndexes[D]);

		if (D == 0)
		{
			auto iter = index->index0.find(query);
			if (iter != index->index0.end())
			{
				auto &value = sharedInfo->mapping.get(iter->second);
				ids.push_back(value.id);
				eds.push_back(0);
			}
			return;
		}

		IDType id[16];
		int result[16];
		int length[16];

		int qlen = strlen(query);
		hash.initQuery(query, qlen);
		auto &partIndex = index->partIndex;

		int count = 0;
		for (int partId = 0; partId < index->PN; partId++)
		{
			for (unsigned lp = 0; lp < partIndex[partId][qlen].size(); lp++)
			{
				int stPos = partIndex[partId][qlen][lp].stPos;
				int pLen = partIndex[partId][qlen][lp].partLen;
				int clen = partIndex[partId][qlen][lp].len;

				auto value = hash.getQueryValue(stPos, stPos + pLen);
				auto inv_it = index->invList[partId][clen].find(value);
				if (inv_it == index->invList[partId][clen].end())
					continue;

				// enumerate all element in inverted list
				for (auto vit = inv_it->second.begin(); vit != inv_it->second.end(); vit++)
				{
					auto &value = sharedInfo->mapping.get(*vit);
					const char *candidate = value.content;
					for (int k = 0; k < clen; k++)
						buffer[k][count] = candidate[k];
					length[count] = clen;
					id[count] = value.id;

					if (++count == 16)
					{
						getED((__m128i*)buffer, query, length, qlen, result, count, D);
						for (int k = 0; k < 16; k++)
						{
							if (result[k] <= D)
							{
								ids.push_back(id[k]);
								eds.push_back(result[k]);
							}
						}
						count = 0;
					}
				}
			}
		}

		if (count != 0)
		{
			getED((__m128i*)buffer, query, length, qlen, result, count, D);
			for (int k = 0; k < count; k++)
			{
				if (result[k] <= D)
				{
					ids.push_back(id[k]);
					eds.push_back(result[k]);
				}
			}
		}

		hash.destroyQuery();
	}
private:
	void getED(__m128i *a, const char *b, int *alen, int blen, int *eds, int count, int D)
	{
		__m128i one = _mm_set1_epi8(1);
		for (int i = 1; i <= blen; i++)
		{
			__m128i bcurrent = _mm_set1_epi8(b[i - 1]);
			for(int j = 1; j <= blen + D; j++)
			{
				/*matrix[i][j] = _mm_add_epi8(matrix[i - 1][j - 1], _mm_cmpeq_epi8(bcurrent, a[j - 1]));
				matrix[i][j] = _mm_min_epi8(matrix[i][j], _mm_min_epi8(matrix[i][j - 1], matrix[i - 1][j]));
				matrix[i][j] = _mm_add_epi8(matrix[i][j], one);*/
				matrix[i][j] = _mm_add_epi8(matrix[i - 1][j - 1], _mm_cmpeq_epi8(bcurrent, a[j - 1]));
				matrix[i][j] = _mm_add_epi8(matrix[i][j], one);
				matrix[i][j] = _mm_min_epu8(matrix[i][j],
						_mm_add_epi8(_mm_min_epu8(matrix[i][j - 1], matrix[i - 1][j]), one));
			}
		}

		for (int k = 0; k < count; k++)
		{
			if (alen[k] <= blen + D)
			{
				eds[k] = ((char*)&(matrix[blen][alen[k]]))[k];
			} else {
				alen[k] = D + 1;
			}
		}
	}

	void getHM(__m128i *a, const char *b, int len, int *hms, int count, int D)
	{
		__m128i result = _mm_set1_epi8(len);
		for (int k = 1; k <= len; k++)
		{
			__m128i bcurrent = _mm_set1_epi8(b[k - 1]);
			result = _mm_add_epi8(result, _mm_cmpeq_epi8(bcurrent, a[k - 1]));
		}

		char *c = (char*)&result;
		for (int k = 0; k < count; k++)
		{
			hms[k] = c[k];
		}
	}

	int D;
	Index *index;
	SharedInfo<IDType> *sharedInfo;
	__m128i matrix[SAFELEN][SAFELEN] __attribute__((aligned (16)));
	char buffer[SAFELEN][16] __attribute__((aligned (16)));
	Hash hash;
};

#endif

