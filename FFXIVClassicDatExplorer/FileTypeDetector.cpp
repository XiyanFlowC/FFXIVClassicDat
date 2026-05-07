#include "FileTypeDetector.h"

#include <cstring>
#include <string>
#include <vector>

#include "../FFXIVClassicDat/ShuffleString.h"

static const uint8_t MICROSOFT_COMPOUND_FILE_HEADER_SIGNATURE[] = {
	0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1
};

static const uint8_t ZIPATCH_MAGIC[12] = {
	0x91, 'Z', 'I', 'P', 'A', 'T', 'C', 'H', 0x0D, 0x0A, 0x1A, 0x0A
};

static std::string XmlOpenTagSimpleCheck(const char* buf, int length)
{
	const char* end = buf + length;
	if (*buf++ != '<') return "";
	std::string ret;
	while (*buf != ' ' && *buf != '\t' && *buf != '\r' && *buf != '\n' && *buf != '>')
	{
		if (buf >= end) return "";
		if ((*buf < 'a' || *buf > 'z') && (*buf < 'A' || *buf > 'Z') &&
			*buf != '-' && *buf != ':')
			return "";
		ret += *buf++;
	}
	return ret;
}

static int XmlCloseTagSimpleCheck(const char* buf, int length, const std::string& target)
{
	const char* end = buf + length - 1;
	while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
		--end;
	if (*end != '>') return 0;
	auto t = target.rbegin();
	while (t != target.rend())
	{
		if (*t++ != *end--) return 0;
	}
	return 1;
}

static int XmlTopLabelCheck(const char* buf, int length)
{
	auto tag = XmlOpenTagSimpleCheck(buf, length);
	if (tag.empty()) return 0;
	return XmlCloseTagSimpleCheck(buf, length, tag);
}

static int CheckXmlType(const uint8_t* data, int length)
{
	if (length >= 3 && memcmp(data, "\xEF\xBB\xBF", 3) == 0)
	{
		std::string str(reinterpret_cast<const char*>(data) + 3, length - 3);
		if (str.starts_with("<?xml"))
		{
			if (str.find("<ssd ") != std::string::npos) return 2;
			return 1;
		}
	}
	else
	{
		if (length >= 5 && memcmp(data, "<?xml", 5) == 0)
		{
			std::string str(reinterpret_cast<const char*>(data), length);
			if (str.find("<ssd ") != std::string::npos) return 2;
			return 1;
		}
		if (XmlTopLabelCheck(reinterpret_cast<const char*>(data), length)) return 1;
	}

	ShuffleString ss;
	std::vector<uint8_t> tmpBuf(length + 1);
	int decLen = ss.Decrypt(const_cast<uint8_t*>(data), length, tmpBuf.data(), static_cast<int>(tmpBuf.size()));
	if (decLen < 0) decLen = length;
	tmpBuf[decLen] = 0;

	if (decLen >= 3 && memcmp(tmpBuf.data(), "\xEF\xBB\xBF", 3) == 0)
	{
		std::string str(reinterpret_cast<const char*>(tmpBuf.data()) + 3, decLen - 3);
		if (str.starts_with("<?xml"))
		{
			if (str.find("<ssd ") != std::string::npos) return 2;
			return 1;
		}
		if (XmlTopLabelCheck(reinterpret_cast<const char*>(tmpBuf.data()) + 3, decLen)) return 1;
	}
	else
	{
		if (decLen >= 5 && memcmp(tmpBuf.data(), "<?xml", 5) == 0)
		{
			std::string str(reinterpret_cast<const char*>(tmpBuf.data()), decLen);
			if (str.find("<ssd ") != std::string::npos) return 2;
			return 1;
		}
		if (XmlTopLabelCheck(reinterpret_cast<const char*>(tmpBuf.data()), decLen)) return 1;
	}

	return 0;
}

FileTypeResult DetectFileTypeDetail(const uint8_t* data, size_t size)
{
	FileTypeResult result;

	if (size < 4)
	{
		result.type = FileType::Binary;
		return result;
	}

	if (memcmp(data, "GTEX", 4) == 0)
	{
		result.type = FileType::GTEX;
		return result;
	}

	if (memcmp(data, "VERS", 4) == 0)
	{
		result.type = FileType::FDT;
		return result;
	}

	if (memcmp(data, "SEDB", 4) == 0)
	{
		result.type = FileType::SEDB;
		return result;
	}

	if (memcmp(data, "SQEX", 4) == 0)
	{
		result.type = FileType::SQWT;
		return result;
	}

	if (memcmp(data, "DDS ", 4) == 0)
	{
		result.type = FileType::DDS;
		return result;
	}

	if (size >= 12 && memcmp(data, ZIPATCH_MAGIC, 12) == 0)
	{
		result.type = FileType::ZiPatch;
		return result;
	}

	if (size >= 20 && memcmp(data, "VfxGraphResourceData", 20) == 0)
	{
		result.type = FileType::VGRD;
		return result;
	}

	if (size >= 20 && memcmp(data, "MapLayoutResourceData", 20) == 0)
	{
		result.type = FileType::MLRD;
		return result;
	}

	if (size >= 8 && memcmp(data, MICROSOFT_COMPOUND_FILE_HEADER_SIGNATURE, 8) == 0)
	{
		result.type = FileType::CFB;
		return result;
	}

	if (size >= 3)
	{
		int xmlType = CheckXmlType(data, static_cast<int>(size));
		if (xmlType == 2)
		{
			result.type = FileType::SSD;
			result.xmlSubType = 2;
			return result;
		}
		if (xmlType == 1)
		{
			result.type = FileType::XML;
			result.xmlSubType = 1;
			return result;
		}
	}

	result.type = FileType::Binary;
	return result;
}

FileType DetectFileType(const uint8_t* data, size_t size)
{
	return DetectFileTypeDetail(data, size).type;
}

std::wstring_view FileTypeName(FileType type)
{
	switch (type)
	{
		case FileType::FDT:     return L"FDT (Font Data Table)";
		case FileType::GTEX:    return L"GTEX (Texture)";
		case FileType::SSD:     return L"SSD (Spreadsheet Data)";
		case FileType::XML:     return L"XML";
		case FileType::DDS:     return L"DDS (Texture)";
		case FileType::SEDB:    return L"SEDB (Sound)";
		case FileType::SQWT:    return L"SQWT (Encrypted UI)";
		case FileType::ZiPatch: return L"ZiPatch (Patch File)";
		case FileType::CFB:     return L"Compound File Binary";
		case FileType::VGRD:    return L"VGRD (VFX Graph)";
		case FileType::MLRD:    return L"MLRD (Map Layout)";
		default:               return L"Binary";
	}
}
