#include "SsdData.h"
#include "DataManager.h"
#include "xybase/Exception/InvalidOperationException.h"

SsdData::SsdData()
	: m_fileId(-1)
{
}

SsdData::SsdData(uint32_t p_fileId)
	: m_fileId(p_fileId)
{
	ParseRaptureSsdData();
}

SsdData::~SsdData()
{
	for (auto &&pair : m_sheets)
	{
		delete pair.second;
	}
}

const Sheet *SsdData::GetSheet(const std::u8string &sheetName)
{
	if (!m_isSsdParsed) throw xybase::InvalidOperationException(L"Ssd data have not been parsed yet!", 86700);

	return m_sheets[sheetName];
	m_isModified = 1;
}

void SsdData::AppendSheet(const std::u8string &sheetName, Sheet *sheet)
{
	if (m_sheets.contains(sheetName)) throw xybase::InvalidOperationException(L"Sheet already exists!", 86701);

	m_sheets[sheetName] = sheet;
	m_isModified = 1;
}

void SsdData::AppendSheetDetermined(const std::u8string &sheetName, Sheet *sheet)
{
	if (m_sheets.contains(sheetName))
	{
		delete m_sheets[sheetName];
	}

	m_sheets[sheetName] = sheet;
	m_isModified = 1;
}

void SsdData::ParseRaptureSsdData()
{
	auto data = DataManager::GetInstance().LoadData(m_fileId);

	m_isSsdParsed = 1;
}
