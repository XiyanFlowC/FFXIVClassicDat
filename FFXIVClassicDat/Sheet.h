#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Sheet 是存储在data种的一类dat。这里是infofile的解析。
 * infofile规定了Sheet的schema，并指定了实际数据的存储位置等控制信息。
 * Sheet通常被Ssd文件所指向。
 */
class Sheet
{
public:
	Sheet(const std::u8string& name, int columnMax, int columnCount, int cache = -1);

	const std::u8string &GetName() const;

	class Schema
	{
		enum class DataType
		{
			SDT_INT8,
			SDT_INT16,
			SDT_INT32,
			SDT_INT64,
			SDT_UINT8,
			SDT_UINT16,
			SDT_UINT32,
			SDT_UINT64,
			SDT_
		};
	};

protected:
	std::u8string m_name;
	std::u8string m_mode;
	std::u8string m_type;
	std::u8string m_lang;
	int m_columnMax;
	int m_columnCount;
	int m_cache;
private:
};

