#include "SqwtFile.h"

#include <filesystem>
#include <fstream>
#include <cstring>

#include "BinaryData.h"
#include "xybase/xystring.h"
#include "xybase/Exception/NotImplementedException.h"
#include "SqwtDecryptUtility.h"


SqwtFile::SqwtFile(std::wstring p_path)
{
	LoadFile(p_path);
}

void SqwtFile::LoadFile(std::wstring p_path)
{
	std::wstring pathSep(L"\\/");
	int pathEnd = p_path.find_last_of(pathSep);
	std::wstring filename;
	if (pathEnd == std::wstring::npos)
		filename = p_path;
	else
		filename = p_path.substr(pathEnd + 1);

	auto u8name = xybase::string::to_utf8(filename);
	BlowFish decryptUtility((char *)u8name.c_str(), u8name.size());

	std::ifstream eye(p_path, std::ios::binary);
	size_t fileSize = std::filesystem::file_size(p_path);
	char *contents = new char[fileSize];
	eye.read(contents, fileSize);
	eye.close();

	SqwtFileHeader *hdr = (SqwtFileHeader *)contents;
	if (memcmp(&hdr->magicHeader, SQEX_MAGIC_HEAD, 4) || fileSize <= 8)
	{
		m_fileContent = BinaryData(contents, fileSize, false);
		return;
	}

	if (fileSize <= 8)
	{
		delete[]contents;
		throw xybase::InvalidParameterException(L"filepath", L"Invalid SQEX file!", 620110);
	}

	size_t trueSize = fileSize - 8;
	char *decryptedContents = new char[trueSize];

	decryptUtility.Decrypt(decryptedContents, contents + 8, trueSize);

	m_fileContent = BinaryData(decryptedContents, trueSize, false);
	delete []contents;
}

void SqwtFile::ParseFile()
{
	throw xybase::NotImplementedException();
}
