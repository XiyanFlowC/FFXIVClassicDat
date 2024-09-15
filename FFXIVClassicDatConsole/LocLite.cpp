#include "LocLite.h"

#include <cstdlib>
#include <cstring>
#include "Config.h"
#include <filesystem>
#include <fstream>

const wchar_t *(*locLiteData)[6];

int wstrcmp(const wchar_t *lhs, const wchar_t *rhs)
{
	while (*lhs && *rhs)
	{
		int diff = *lhs++ - *rhs++;
		if (diff) return diff;
	}
	return *rhs - *lhs;
}

const wchar_t *T(const wchar_t *t)
{
	const wchar_t *(*ptr)[6] = locLiteData;
	while ((*ptr)[0])
	{
		if (wstrcmp(t, (*ptr)[0]) == 0)
			return (*ptr)[Config::GetInstance().m_lang] ? (*ptr)[Config::GetInstance().m_lang] : t;
		ptr++;
	}
	return t;
}

const wchar_t *I(int i)
{
	return locLiteData[i][Config::GetInstance().m_lang] ? locLiteData[i][Config::GetInstance().m_lang] : locLiteData[i][0];
}

void SetTTable(const wchar_t *(*tbl)[6])
{
	locLiteData = tbl;
}

void LoadTTable(const wchar_t *fileName)
{
	auto size = std::filesystem::file_size(fileName);
	locLiteData = (const wchar_t *(*)[6])malloc(size);
	if (!locLiteData) return;
	std::ifstream eye{ fileName, std::ios::binary };
	int entCount = 0;
	eye.read((char *)&entCount, 4);
	eye.read((char *)locLiteData, size - 4);
	for (int i = 0; i < entCount; ++i)
	{
		for (int j = 0; j < 6; ++j)
		{
			if (locLiteData[i][j])
				locLiteData[i][j] = (const wchar_t *)((char *)locLiteData[i][j] + (unsigned long long)locLiteData);
		}
	}
}

void UnloadTTable()
{
	free(locLiteData);
}
