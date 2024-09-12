#include "CsvUtility.h"

#include "xybase/xystring.h"

CsvGenerateUtility::CsvGenerateUtility(bool p_addBom)
{
	if (p_addBom)
		m_sb += xybase::string::to_utf8(0xFEFF);
}

void CsvGenerateUtility::NewSheet(bool p_addBom)
{
	m_sb.Clear();
	if (p_addBom)
		m_sb += xybase::string::to_utf8(0xFEFF);
}

void CsvGenerateUtility::AddCell(const std::u8string &p_str)
{
	if (m_cellCount++) m_sb.Append(',');

	if (p_str.find_first_of(u8"\n\r\",") != std::u8string::npos)
	{
		m_sb += '"';
		for (auto &&ch : p_str)
		{
			if (ch == '"') m_sb += u8"\"\"";
			else m_sb += ch;
		}
		m_sb += '"';
	}
	else
	{
		m_sb.Append(p_str.c_str());
	}
}

int CsvGenerateUtility::NewRow()
{
	m_sb += u8"\n";
	int ret = m_cellCount;
	m_cellCount = 0;
	return ret;
}

std::u8string CsvGenerateUtility::ToString()
{
	return m_sb.ToString();
}
