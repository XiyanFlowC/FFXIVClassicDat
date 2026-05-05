#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <list>

class Sheet;

/**
 * @brief SSD 数据解析器 — 解析索引数据表的 SSD 文件（XML）
 * 
 * 包括数据表的结构定义、数据块信息、以及数据块的启用提示等。提供对解析结果的访问接口。
 */
class SsdData
{
public:
	SsdData();

	SsdData(uint32_t fileId, const std::u8string &language);

	SsdData(const std::wstring &path, const std::u8string &language);

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
	uint8_t m_isSsdParsed : 1 = 0;
	uint8_t m_isModified : 1 = 0;
};
