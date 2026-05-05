#include "SedbFile.h"
#include <cstdlib>
#include <cstring>
#include "xybase/Exception/InvalidParameterException.h"
#include "DataManager.h"
#include "BinaryData.h"

SedbFile::SedbFile(const char* buffer)
{
	Load(buffer);
}

SedbFile::SedbFile(uint32_t fileId)
{
	auto ret = DataManager::GetInstance().LoadData(fileId);
	if (ret.GetLength() < sizeof(SedbFile::SedbHeader))
	{
		throw xybase::InvalidParameterException(L"fileId", L"File size is smaller than SEDB header.", 145701);
	}
	Load(static_cast<char *>(ret.GetData()));
}

void SedbFile::Load(const char* buffer)
{
	SedbHeader *header = (SedbHeader *)buffer;
	if (memcmp(header->magic, "SEDB", 4) != 0)
	{
		throw xybase::InvalidParameterException(L"buffer", L"Invalid SEDB file header.", 145702);
	}
	m_data = new char[header->fileSize];
	memcpy(m_data, buffer, header->fileSize);
	m_payload = m_data + header->headersize;
}
