#include "SqwtFile.h"

#include <filesystem>
#include <fstream>
#include <cstring>

#include "BinaryData.h"
#include "xybase/xystring.h"
#include "xybase/Exception/NotImplementedException.h"
#include "SqwtDecryptUtility.h"


SqwtFile::SqwtFile(std::wstring path)
{
	LoadFile(path);
}

void SqwtFile::LoadFile(std::wstring path)
{
	std::wstring pathSep(L"\\/");
	int pathEnd = path.find_last_of(pathSep);
	std::wstring filename;
	if (pathEnd == std::wstring::npos)
		filename = path;
	else
		filename = path.substr(pathEnd + 1);

	auto u8name = xybase::string::to_utf8(filename);
	SqwtDecryptUtility decryptUtility((char *)u8name.c_str(), u8name.size());

	std::ifstream eye(path, std::ios::binary);
	size_t fileSize = std::filesystem::file_size(path);
	char *contents = new char[fileSize];
	eye.read(contents, fileSize);
	eye.close();

	SqwtFileHeader *hdr = (SqwtFileHeader *)contents;
	if (memcmp(&hdr->magicHeader, SQEX_MAGIC_HEAD, 4) || fileSize <= 8)
	{
		FileContent = BinaryData(contents, fileSize, false);
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

	FileContent = BinaryData(decryptedContents, trueSize, false);
	delete []contents;
}

void SqwtFile::ParseFile()
{
	throw xybase::NotImplementedException();
}
