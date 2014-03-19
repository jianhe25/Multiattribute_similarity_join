#ifndef LIB_COMMON_H
#define LIB_COMMON_H

#include <algorithm>
#include <x86intrin.h>
#include <vector>
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <cstring>

const int MAXLEN = 100;
const int SAFELEN = MAXLEN + 4;
const int MAXED = 4;
const int SAFEED = MAXED + 2;
const int MAXID = 2e6;

template <typename T>
T min2(const T &t1, const T &t2)
{
	return t1 < t2 ? t1 : t2;
}

template <typename T>
T max2(const T &t1, const T &t2)
{
	return t1 < t2 ? t2 : t1;
}

template <typename T>
T min3(const T &t1, const T &t2, const T &t3)
{
	return min2(t1, min2(t2, t3));
}

template <typename T>
T max3(const T &t1, const T &t2, const T &t3)
{
	return max2(t1, max2(t2, t3));
}

template<typename T>
void alloc1d(T *&p, int m)
{
	p = new T[m];
}

template<typename T>
void alloc2d(T **&p, int m, int n)
{
	p = new T*[m];
	T *t = new T[m * n];
	for (int i = 0; i < m; i++)
		p[i] = t + i * n;
}

template<typename T>
void alloc3d(T ***&p, int t, int m, int n)
{
	p = new T**[t];
	for (int i = 0; i < t; i++)
		alloc2d(p[i], m, n);
}

template<typename T>
void free1d(T *p)
{
	delete[] p;
}

template<typename T>
void free2d(T **p)
{
	delete[] p[0];
	delete[] p;
}

template<typename T>
void free3d(T ***p, int t)
{
	for (int i = 0; i < t; i++)
		free2d(p[i]);
	delete[] p;
}

#endif

