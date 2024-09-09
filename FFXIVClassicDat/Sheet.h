#pragma once

#include <cstdint>
#include <string>
#include <map>

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
	public:
		enum class DataType
		{
			SDT_INVALID,
			SDT_S8,
			SDT_U8,
			SDT_S16,
			SDT_U16,
			SDT_S32,
			SDT_U32,
			SDT_BOOL,
			SDT_F16,
			SDT_F32,
			SDT_STR,
			SDT_MAX
		};

		Schema();

		void SetType(int p_index, const char8_t *p_type);

		void RemoveType(int p_index);

		DataType QueryType(int p_index);

	private:
		std::map<int, DataType> m_schema;
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

