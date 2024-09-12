#pragma once

#include <string>

#include "BinaryData.h"
#include "xybase/BinaryStream.h"

/**
 * @brief 从Data文件夹读取数据文件所用的类
 */
class DataManager
{
private:
	DataManager() {};

public:
	static DataManager &GetInstance();

	std::wstring m_basePath;

	BinaryData LoadData(uint32_t p_id);

	xybase::BinaryStream *NewDataStream(uint32_t p_id, const wchar_t *p_mode);

	void SaveData(uint32_t p_id, const BinaryData &p_data);
};

