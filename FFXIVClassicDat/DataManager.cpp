#include "DataManager.h"

#include <format>
#include <filesystem>
#include <fstream>

#include "BinaryData.h"
#include "xybase/BinaryStream.h"
#include "xybase/Exception/InvalidParameterException.h"

DataManager &DataManager::GetInstance()
{
	static DataManager _inst;
	return _inst;
}

BinaryData DataManager::LoadData(uint32_t p_id)
{
	std::wstring path = std::format(L"{}/{:02X}/{:02X}/{:02X}/{:02X}.DAT", m_basePath, p_id >> 24, (p_id >> 16) & 0xFF, (p_id >> 8) & 0xFF, p_id & 0xFF);

	if (!std::filesystem::exists(path)) throw xybase::InvalidParameterException(L"p_id", L"Specified file not found.", 87700);

	size_t fileSize = std::filesystem::file_size(path);
	char *buffer = new char[fileSize];
	std::ifstream eye(path, std::ios::binary);
	eye.read(buffer, fileSize);
	eye.close();
	return BinaryData(buffer, fileSize, false);
}

xybase::BinaryStream *DataManager::NewDataStream(uint32_t p_id, const wchar_t *p_mode)
{
	std::wstring path = std::format(L"{}/{:02X}/{:02X}/{:02X}/{:02X}.DAT", m_basePath, p_id >> 24, (p_id >> 16) & 0xFF, (p_id >> 8) & 0xFF, p_id & 0xFF);

	if (!std::filesystem::exists(path)) throw xybase::InvalidParameterException(L"p_id", L"Specified file not found.", 87700);

	return new xybase::BinaryStream(path, p_mode);
}

void DataManager::SaveData(uint32_t p_id, const BinaryData &p_data)
{
	std::wstring path = std::format(L"{}/{:02X}/{:02X}/{:02X}/{:02X}.DAT", m_basePath, p_id >> 24, (p_id >> 16) & 0xFF, (p_id >> 8) & 0xFF, p_id & 0xFF);
	std::wstring dir = std::format(L"{}/{:02X}/{:02X}/{:02X}/", m_basePath, p_id >> 24, (p_id >> 16) & 0xFF, (p_id >> 8) & 0xFF);

	if (!std::filesystem::exists(dir))
	{
		std::filesystem::create_directories(dir);
	}

	std::ofstream pen(path, std::ios::binary | std::ios::trunc);
	pen.write((char *)p_data.GetData(), p_data.GetLength());
	pen.close();
}

