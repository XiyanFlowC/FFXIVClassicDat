#include "SsdData.h"
#include "DataManager.h"
#include "xybase/Exception/InvalidOperationException.h"

SsdData::SsdData()
	: m_fileId(-1), m_language(u8"ja")
{
}

SsdData::SsdData(uint32_t p_fileId, const std::u8string &p_language)
	: m_fileId(p_fileId), m_language(p_language)
{
	ParseRaptureSsdData(m_fileId);
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

std::list<Sheet *> SsdData::GetAllSheets() const
{
	std::list<Sheet *> ret;
	for (auto &&pair : m_sheets)
	{
		ret.push_back(pair.second);
	}
	return ret;
}

#include "ShuffleString.h"

void SsdData::ParseRaptureSsdData(uint32_t id)
{
	auto data = DataManager::GetInstance().LoadData(id);

	// ShuffleString
	ShuffleString ss;
	int length = ss.Decrypt(data.GetData(), data.GetLength(), data.GetData(), data.GetLength());
	if (length < 0) length = data.GetLength();

	// utf-8 BOM check
	if (!memcmp(data.GetData(), "\xEF\xBB\xBF", 3))
	{
		ParseRaptureSsdData((char8_t *)data.GetData() + 3, length - 3);
	}
	else
	{
		ParseRaptureSsdData((char8_t *)data.GetData(), length);
	}

	m_isSsdParsed = 1;
}

#include "xybase/Xml/XmlNode.h"
#include "xybase/Xml/XmlParser.h"

void ParseSheetTag(Sheet *target, xybase::xml::XmlNode node)
{

}

void SsdData::ParseRaptureSsdData(const char8_t *xml, int length)
{
	xybase::xml::XmlParser<xybase::xml::XmlNode, char8_t> parser{};
	xybase::xml::XmlNode root = parser.Parse(xml);

	if (root.GetName() != u"ssd" || root.GetAttribute(u"version") != u"0.1")
		throw xybase::InvalidParameterException(L"xml", L"Invalid ssd format or unsupported version.", 458000);

	for (auto &&child : root.GetChildren())
	{
		if (child.GetName() == u"sheet")
		{
			std::u8string name = xybase::string::to_utf8(child.GetAttribute(u"name"));
			std::u16string infoFile = child.GetAttribute(u"infofile");
			// 引用其他的文件
			if (infoFile != u"")
			{
				ParseRaptureSsdData(xybase::string::stoi<char16_t>(infoFile));
			}

			// 非引用，解析数据
			std::u8string mode = xybase::string::to_utf8(child.GetAttribute(u"mode"));
			int columnMax = xybase::string::pint<char16_t>(child.GetAttribute(u"column_max"));
			int columnCount = xybase::string::pint<char16_t>(child.GetAttribute(u"column_count"));
			int cache = xybase::string::pint<char16_t>(child.GetAttribute(u"cache"));
			std::u8string type = xybase::string::to_utf8(child.GetAttribute(u"type"));
			std::u8string lang = xybase::string::to_utf8(child.GetAttribute(u"lang"));

			// 该表是当前指定的语言的表
			if (lang == m_language || lang == u8"")
			{
				Sheet *sheet = new Sheet(name, columnMax, columnCount, cache);
				m_sheets[name] = sheet;
			}
			// 否则，忽略此记录
		}
		else
			throw xybase::InvalidParameterException(L"xml", L"Unsupported ssd definition.", 458001);
	}
}
