#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <list>

class Sheet;

class SsdData
{
public:
	SsdData();

	SsdData(uint32_t p_fileId, const std::u8string &p_language);

	SsdData(const std::wstring &path, const std::u8string &p_language);

	~SsdData();

	Sheet * GetSheet(const std::u8string &sheetName) const;

	void AppendSheet(const std::u8string &sheetName, Sheet *sheet);

	void AppendSheetDetermined(const std::u8string &sheetName, Sheet *sheet);

	std::list<Sheet *> GetAllSheets() const;

	/**
	 * @brief 指示是否可以解析 infofile 属性。若为 false 则忽略 infofile。
	 */
	bool m_recursive = true;
private:
	void ParseRaptureSsdData(uint32_t id);

	void ParseRaptureSsdData(std::wstring path);

	void ParseRaptureSsdData(const char8_t *xml, int length);

	std::map<std::u8string, Sheet *> m_sheets;
	std::u8string m_language;
	uint32_t m_fileId;
	// uint8_t m_isSheetsParsed : 1 = 0;
	uint8_t m_isSsdParsed : 1 = 0;
	uint8_t m_isModified : 1 = 0;
};

