#pragma once

#include "xybase/StringBuilder.h"

class CsvGenerateUtility
{
public:

	CsvGenerateUtility(bool p_addBom = true);

	void NewSheet(bool p_addBom);

	void AddCell(const std::u8string &p_str);

	int NewRow();

	std::u8string ToString();

private:
	int m_cellCount = 0;

	xybase::StringBuilder<char8_t> m_sb;
};

