#include "LuaScriptFile.h"

#include <filesystem>
#include <fstream>
#include <cstring>
#include "xybase/xystring.h"
#include "SimpleString.h"
#include "BinaryData.h"

std::wstring LuaScriptFile::s_subAlphabet = L"0123456789abcdefghijklmnopqrstuvwxyz";
std::wstring LuaScriptFile::s_revAlphabet = L"jihgfedcba9876543210zyxwvutsrqponmlk";

LuaScriptFile::LuaScriptFile(const std::wstring &scriptBasePath)
{
	m_basePath = scriptBasePath;
	std::wstring staticActorSanPath = scriptBasePath + FileNameCipher(L"StaticActor") + L".san";
	size_t length = std::filesystem::file_size(staticActorSanPath);
	std::ifstream eye(staticActorSanPath, std::ios::binary);
	SanHeader header;
	eye.read((char *) &header, sizeof(SanHeader));
	if (memcmp(&header.magic, "sane", 4))
	{
		throw xybase::InvalidParameterException(L"staticActorSanPath", L"Magic Header Verification failed.", 187200);
	}
	size_t recordLength = length - sizeof(SanHeader);
	char *buffer = new char[recordLength];
	eye.read(buffer, recordLength);

	/* Decrypt if first byte is not 0xFF, then no encryption here */
	SimpleString ss;
	int trueLength = ss.Decrypt(buffer, recordLength, buffer, recordLength);
	if (trueLength < 0) trueLength = recordLength;

	char *cur = buffer;
	while (cur <= buffer + trueLength)
	{
		Actor *actor = (Actor *)cur;
		std::u8string name{ (char8_t *)actor->name };
		m_actors[actor->id] = name;
		size_t alignedLength = (name.size() + 1) + 3 & ~3;
		cur += 4 + alignedLength;
	}


	delete[] buffer;
	eye.close();
}

std::wstring LuaScriptFile::FileNameCipher(std::wstring_view fileName)
{
	std::wstring ret;
	std::wstring convertedFileName = xybase::string::to_lower(std::wstring{ fileName });
	for (auto &&ch : convertedFileName)
	{
		size_t code = s_subAlphabet.find(ch);
		if (code != std::wstring::npos)
		{
			ret += s_revAlphabet[code];
		}
		else
			ret += ch;
	}
	return ret;
}

std::wstring LuaScriptFile::FileNameDecipher(std::wstring_view fileName)
{
	std::wstring ret;
	for (auto &&ch : fileName)
	{
		size_t code = s_revAlphabet.find(ch);
		if (code != std::wstring::npos)
		{
			ret += s_subAlphabet[code];
		}
		else
			ret += ch;
	}
	return ret;
}

BinaryData LuaScriptFile::GetLuacDataByPath(const std::wstring &path)
{
	if (!std::filesystem::exists(path))
		throw xybase::InvalidParameterException(L"path", L"Specifed actor not found.", 107210);

	size_t length = std::filesystem::file_size(path);
	char *contents = new char[length];
	std::ifstream eye(path, std::ios::binary);
	eye.read(contents, length);
	eye.close();

	LpdHeader *hdr = (LpdHeader *)contents;

	if (memcmp(hdr->magic, "rle\x0c", 4))
	{
		throw xybase::InvalidParameterException(L"path", L"Not a valid lpd.", 107211);
	}

	BinaryData ret(hdr->fileSize);

	SimpleString ss;
	ss.Decrypt(hdr->script, length - 12, ret.GetData(), ret.GetLength());

	delete[] contents;
	return ret;
}

BinaryData LuaScriptFile::GetLuacDataByName(const std::u8string &actorName)
{
	std::wstring lpdPath = m_basePath + L"\\" + FileNameCipher(xybase::string::to_wstring(actorName)) + L"_p.le.lpd";
	return GetLuacDataByPath(lpdPath);
}


BinaryData LuaScriptFile::GetLuacDataByActorId(uint32_t id)
{
	return GetLuacDataByName(m_actors.find(id)->second);
}
