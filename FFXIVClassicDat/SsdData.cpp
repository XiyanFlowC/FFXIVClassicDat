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

#include "ShuffleString.h"

void SsdData::ParseRaptureSsdData()
{
	auto data = DataManager::GetInstance().LoadData(m_fileId);

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
			uint32_t infofile = xybase::string::stoi<char16_t>(child.GetAttribute(u"infofile"));

			Sheet *sheet = new Sheet(infofile);
			m_sheets[name] = sheet;
		}
		else
			throw xybase::InvalidParameterException(L"xml", L"Unsupported ssd definition.", 458001);
	}
}
