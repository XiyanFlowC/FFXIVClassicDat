#include "LuaScriptFile.h"

#include <filesystem>
#include <fstream>
#include <cstring>
#include "xybase/xystring.h"

std::wstring LuaScriptFile::subalphabet = L"0123456789abcdefghijklmnopqrstuvwxyz";
std::wstring LuaScriptFile::revalphabet = L"jihgfedcba9876543210zyxwvutsrqponmlk";

LuaScriptFile::LuaScriptFile(const std::wstring &p_scriptBasePath)
{
	basePath = p_scriptBasePath;
	std::wstring p_staticActorSanPath = p_scriptBasePath + FileNameCipher(L"StaticActor") + L".san";
	size_t length = std::filesystem::file_size(p_staticActorSanPath);
	std::ifstream eye(p_staticActorSanPath, std::ios::binary);
	SanHeader header;
	eye.read((char *) &header, sizeof(SanHeader));
	if (memcmp(&header.magic, "sane", 4))
	{
		throw xybase::InvalidParameterException(L"p_staticActorSanPath", L"Magic Header Verification failed.", 187200);
	}
	size_t recordLength = length - sizeof(SanHeader);
	char *buffer = new char[recordLength];
	eye.read(buffer, recordLength);

	for (int i = 0; i < recordLength; ++i)
	{
		buffer[i] ^= 0x73;
	}

	char *cur = buffer;
	while (cur <= buffer + length)
	{
		Actor *actor = (Actor *)cur;
		std::u8string name{ (char8_t *)actor->name };
		actors[actor->id] = name;
		size_t alignedLength = (name.size() + 1) + 3 & ~3;
		cur += 4 + alignedLength;
	}


	delete[] buffer;
	eye.close();
}

std::wstring LuaScriptFile::FileNameCipher(std::wstring_view p_fileName)
{
	std::wstring ret;
	std::wstring fileName = xybase::string::to_lower(std::wstring{ p_fileName });
	for (auto &&ch : fileName)
	{
		size_t code = subalphabet.find(ch);
		if (code != std::wstring::npos)
		{
			ret += revalphabet[code];
		}
		else
			ret += ch;
	}
	return ret;
}

std::wstring LuaScriptFile::FileNameDecipher(std::wstring_view p_fileName)
{
	std::wstring ret;
	for (auto &&ch : p_fileName)
	{
		size_t code = revalphabet.find(ch);
		if (code != std::wstring::npos)
		{
			ret += subalphabet[code];
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

	BinaryData ret(&hdr->script, hdr->fileSize);

	char *ptr = (char *)ret.GetData();
	for (int i = 0; i < ret.GetLength(); ++i)
	{
		*ptr++ ^= 0x73;
	}

	delete[] contents;
	return ret;
}

BinaryData LuaScriptFile::GetLuacDataByName(const std::u8string &actorName)
{
	std::wstring lpdPath = basePath + L"\\" + FileNameCipher(xybase::string::to_wstring(actorName)) + L"_p.le.lpd";
	return GetLuacDataByPath(lpdPath);
}


BinaryData LuaScriptFile::GetLuacDataByActorId(uint32_t id)
{
	GetLuacDataByName(actors.find(id)->second);
}
