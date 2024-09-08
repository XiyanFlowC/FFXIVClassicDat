#pragma once

#include <cstdint>
#include <map>
#include <string>
#include "Sheet.h"

class SsdData
{
public:
	SsdData();

	SsdData(uint32_t p_fileId);

	~SsdData();

	const Sheet *GetSheet(const std::u8string &sheetName);

	void AppendSheet(const std::u8string &sheetName, Sheet *sheet);

	void AppendSheetDetermined(const std::u8string &sheetName, Sheet *sheet);
private:
	void ParseRaptureSsdData();

	std::map<std::u8string, Sheet *> m_sheets;
	uint32_t m_fileId;
	// uint8_t m_isSheetsParsed : 1 = 0;
	uint8_t m_isSsdParsed : 1 = 0;
	uint8_t m_isModified : 1 = 0;
};

