#pragma once

#include <cstdint>
#include <string>

#include "xybase/Exception/RuntimeException.h"

namespace xybase
{
	class BinaryStream;
}
class BinaryData;

/**
 * @brief 数据文件管理器，用于从 Data 文件夹中读取数据文件。
 */
class DataManager
{
private:
	DataManager() : m_traceAccess(false) {};

public:
	class FileMissingException : public xybase::RuntimeException
	{
	public:
		virtual ~FileMissingException() {};

		FileMissingException(uint32_t p_missingFileId);

		uint32_t GetFileId();
	protected:
		uint32_t m_fileId;
	};

	static DataManager &GetInstance();

	std::wstring m_basePath;

	bool m_traceAccess;

	BinaryData LoadData(uint32_t p_id);

	xybase::BinaryStream *NewDataStream(uint32_t p_id, const wchar_t *p_mode);

	std::wstring BuildDataPath(uint32_t p_id);

	void SaveData(uint32_t p_id, const BinaryData &p_data);
};
