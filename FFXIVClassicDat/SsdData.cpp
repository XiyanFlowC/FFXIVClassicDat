#include "SsdData.h"
#include "BinaryData.h"
#include "DataManager.h"
#include "Sheet.h"
#include "xybase/Exception/InvalidOperationException.h"
#include <cassert>

SsdData::SsdData()
	: m_fileId(-1), m_language(u8"ja")
{
}

SsdData::SsdData(uint32_t p_fileId, const std::u8string &p_language)
	: m_fileId(p_fileId), m_language(p_language)
{
	ParseRaptureSsdData(m_fileId);
}

SsdData::SsdData(const std::wstring &path, const std::u8string &p_language)
	: m_fileId(-1), m_language(p_language)
{
	ParseRaptureSsdData(path);
}

SsdData::~SsdData()
{
	for (auto &&pair : m_sheets)
	{
		delete pair.second;
	}
}

Sheet * SsdData::GetSheet(const std::u8string &sheetName) const
{
	if (!m_isSsdParsed) throw xybase::InvalidOperationException(L"Ssd data have not been parsed yet!", 86700);

	if (!m_sheets.contains(sheetName)) return nullptr;

	return m_sheets.find(sheetName)->second;
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
#include <filesystem>

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

void SsdData::ParseRaptureSsdData(std::wstring path)
{
	BinaryData data(std::filesystem::file_size(path));
	xybase::BinaryStream eye(path, L"rb");
	eye.ReadBytes((char *)data.GetData(), data.GetLength());
	eye.Close();

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

// 虽然這個函數分配紿Sheet可以讓結構更清晰, 但是考慮到Sheet的职能不宜超过索引數据
// 且在SSD中明確表示了Sheet之定義爲SSD文件之一部分. 故而, 將Sheet之Xml定義解析置
// 於此処.
void ParseSheetTag(Sheet *target, const xybase::xml::XmlNode &node)
{
	for (auto child : node.GetChildren())
	{
		auto nodeName = child.GetName();
		if (nodeName == u"type")
		{
			for (auto param : child.GetChildren())
			{
				if (param.GetName() != u"param")
				{
					throw xybase::InvalidParameterException(L"node", L"Ill-formed type element.", 14021);
				}
				target->GetSchema().Append(xybase::string::to_utf8(param.GetChildren().begin()->GetText()));
			}
		}
		else if (nodeName == u"index")
		{
			for (auto param : child.GetChildren())
			{
				if (param.GetName() != u"param")
				{
					throw xybase::InvalidParameterException(L"node", L"Ill-formed type element.", 14022);
				}
				target->AppendIndex(xybase::string::stoi(param.GetChildren().begin()->GetText()));
			}
		}
		else if (nodeName == u"block")
		{
			for (auto file : child.GetChildren())
			{
				if (file.name != u"file")
					throw xybase::InvalidParameterException(L"node", L"Ill-formed block element.", 14023);
				Sheet::BlockInfo block;
				block.begin = xybase::string::stoi(file.GetAttribute(u"begin"));
				block.count = xybase::string::stoi(file.GetAttribute(u"count"));
				block.offset = xybase::string::stoi(file.GetAttribute(u"offset"));
				block.enable = xybase::string::stoi(file.GetAttribute(u"enable"));
				block.data = xybase::string::stoi(file.GetChildren().begin()->GetText());
				target->AppendBlock(block);
			}
		}
		else
			throw xybase::InvalidParameterException(L"node", L"Unknown node for sheet.", 14020);
	}
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
				if (m_recursive)
					ParseRaptureSsdData(xybase::string::stoi<char16_t>(infoFile));
				continue;
			}

			// 非引用，解析数据
			std::u8string mode = xybase::string::to_utf8(child.GetAttribute(u"mode"));
			int columnMax = xybase::string::pint<char16_t>(child.GetAttribute(u"column_max"));
			int columnCount = xybase::string::pint<char16_t>(child.GetAttribute(u"column_count"));
			int cache = xybase::string::pint<char16_t>(child.GetAttribute(u"cache"));
			std::u8string type = xybase::string::to_utf8(child.GetAttribute(u"type"));
			std::u8string lang = xybase::string::to_utf8(child.GetAttribute(u"lang"));
			std::u8string param = xybase::string::to_utf8(child.GetAttribute(u"param"));

			// 该表是当前指定的语言的表
			if (lang == m_language || lang == u8"")
			{
				Sheet *sheet = new Sheet(name, columnMax, columnCount, cache, type, lang, param);
				ParseSheetTag(sheet, child);
				
				assert(!m_sheets.contains(name));
				m_sheets[name] = sheet;
			}
			// 否则，忽略此记录
		}
		else
			throw xybase::InvalidParameterException(L"xml", L"Unsupported ssd definition.", 458001);
	}
}
