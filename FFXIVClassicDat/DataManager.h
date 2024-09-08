#pragma once

#include <string>

#include "BinaryData.h"

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

	void SaveData(uint32_t p_id, const BinaryData &p_data);
};

