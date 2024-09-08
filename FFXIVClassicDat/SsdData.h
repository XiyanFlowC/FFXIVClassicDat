#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <list>
#include "Sheet.h"

class SsdData
{
public:
	SsdData();

	SsdData(uint32_t p_fileId, const std::u8string &p_language);

	~SsdData();

	const Sheet *GetSheet(const std::u8string &sheetName);

	void AppendSheet(const std::u8string &sheetName, Sheet *sheet);

	void AppendSheetDetermined(const std::u8string &sheetName, Sheet *sheet);

	std::list<Sheet *> GetAllSheets() const;
private:
	void ParseRaptureSsdData(uint32_t id);

	void ParseRaptureSsdData(const char8_t *xml, int length);

	std::map<std::u8string, Sheet *> m_sheets;
	std::u8string m_language;
	uint32_t m_fileId;
	// uint8_t m_isSheetsParsed : 1 = 0;
	uint8_t m_isSsdParsed : 1 = 0;
	uint8_t m_isModified : 1 = 0;
};

