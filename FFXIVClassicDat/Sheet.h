#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <map>
#include <list>
#include <vector>
#include "xybase/xystring.h"
#include "xybase/Xml/XmlNode.h"
#include "xybase/BinaryStream.h"

/**
 * @brief Sheet 是存储在data种的一类dat。这里是infofile的解析。
 * infofile规定了Sheet的schema，并指定了实际数据的存储位置等控制信息。
 * Sheet由Ssd文件所定义。
 */
class Sheet
{
public:
	bool m_cfgIgnoreEnableIndication = false;

	typedef int DataType;
	
	static const DataType 
		SDT_MASK_TYPE = 0xF00,
		SDT_FLAG_INTEGER = 0x100,
		SDT_FLAG_FLOAT = 0x200,
		SDT_FLAG_BOOL = 0x400,
		SDT_FLAG_STR = 0x800,

		SDT_MASK_WIDTH = 0x7,
		SDT_FLAG_8BIT = 0x1,
		SDT_FLAG_16BIT = 0x2,
		SDT_FLAG_32BIT = 0x4,

		SDT_MASK_SIGN = 0x8,
		SDT_FLAG_UNSIGNED = 0x0,
		SDT_FLAG_SIGNED = 0x8,

		SDT_INVALID = 0x0,
		SDT_U8 = SDT_FLAG_8BIT | SDT_FLAG_INTEGER | SDT_FLAG_UNSIGNED,
		SDT_S8 = SDT_FLAG_8BIT | SDT_FLAG_INTEGER | SDT_FLAG_SIGNED,
		SDT_U16 = SDT_FLAG_16BIT | SDT_FLAG_INTEGER | SDT_FLAG_UNSIGNED,
		SDT_S16 = SDT_FLAG_16BIT | SDT_FLAG_INTEGER | SDT_FLAG_SIGNED,
		SDT_U32 = SDT_FLAG_32BIT | SDT_FLAG_INTEGER | SDT_FLAG_UNSIGNED,
		SDT_S32 = SDT_FLAG_32BIT | SDT_FLAG_INTEGER | SDT_FLAG_SIGNED,
		SDT_F16 = SDT_FLAG_16BIT | SDT_FLAG_FLOAT | SDT_FLAG_SIGNED,
		SDT_F32 = SDT_FLAG_32BIT | SDT_FLAG_FLOAT | SDT_FLAG_SIGNED,
		SDT_BOOL = SDT_FLAG_8BIT | SDT_FLAG_BOOL,
		SDT_STR = SDT_FLAG_STR;

	Sheet(const std::u8string& name, int columnMax, int columnCount, int cache, const std::u8string &type, const std::u8string &lang);

	Sheet(const Sheet &) = delete;

	Sheet(Sheet && p_movee) noexcept;

	~Sheet();

	const std::u8string &GetName() const;

	struct BlockInfo
	{
		// 启用的项
		uint32_t enable;
		// 各项在数据文件的偏移
		uint32_t offset;
		// 偏移
		uint32_t data;
		// 该块的起始位置
		int begin;
		// 该块的数目
		int count;
	};

	/**
	 * @brief 单元格数据。
	 */
	class Cell
	{
		void CheckDataStatus(DataType flags)
		{
			if ((flags & m_type))
			{
				throw xybase::InvalidOperationException(L"Cannot get value as specified type due to incompatibility.", 95003);
			}
		}
	public:

		Cell();

		Cell(DataType p_type);

		Cell(const Cell &p_pat);

		Cell(Cell &&p_movee) noexcept;

		const Cell &operator=(const Cell &p_rval);

		template<typename T>
		T Get();

		template<>
		int Get()
		{
			CheckDataStatus(SDT_FLAG_INTEGER);
			return m_plainValue.i_val;
		}

		template<>
		unsigned int Get()
		{
			CheckDataStatus(SDT_FLAG_INTEGER);
			return m_plainValue.u_val;
		}

		template<>
		float Get()
		{
			CheckDataStatus(SDT_FLAG_FLOAT);
			return m_plainValue.f_val;
		}

		template<>
		bool Get()
		{
			CheckDataStatus(SDT_FLAG_BOOL);
			return m_plainValue.b_val;
		}

		template<>
		std::u8string Get()
		{
			CheckDataStatus(SDT_FLAG_STR);
			return m_str;
		}

		template<typename T>
		void Set(T value);

		template<>
		void Set(int value)
		{
			CheckDataStatus(SDT_FLAG_INTEGER);
			m_plainValue.i_val = value;
		}

		template<>
		void Set(unsigned int value)
		{
			CheckDataStatus(SDT_FLAG_INTEGER);
			m_plainValue.u_val = value;
		}

		template<>
		void Set(float value)
		{
			CheckDataStatus(SDT_FLAG_FLOAT);
			m_plainValue.f_val = value;
		}

		template<>
		void Set(bool value)
		{
			CheckDataStatus(SDT_FLAG_BOOL);
			m_plainValue.b_val = value;
		}

		template<>
		void Set(std::u8string_view value)
		{
			CheckDataStatus(SDT_FLAG_STR);
			m_str = value;
		}

		void SetString(std::u8string_view value)
		{
			CheckDataStatus(SDT_FLAG_STR);
			m_str = value;
		}

		std::string ToString()
		{
			if (m_type & SDT_FLAG_INTEGER)
			{
				if (m_type & SDT_FLAG_SIGNED)
				{
					if (m_plainValue.i_val < 0)
						return "-" + xybase::string::itos(-m_plainValue.i_val);
					else
						return xybase::string::itos(m_plainValue.i_val);
				}
				else
					return xybase::string::itos(m_plainValue.u_val);
			}
			else if (m_type & SDT_FLAG_BOOL)
			{
				return m_plainValue.b_val ? "true" : "false";
			}
			else if (m_type & SDT_FLAG_FLOAT)
			{
				return std::to_string(m_plainValue.f_val);
			}
			else if (m_type & SDT_FLAG_STR)
			{
				return xybase::string::to_string(m_str);
			}
			else abort();
		}
	protected:
		DataType m_type;
		union
		{
			int i_val;
			unsigned int u_val;
			float f_val;
			bool b_val;
		} m_plainValue;

		std::u8string m_str;
	};

	class Row
	{
	public:
		Row(int columnCount, int* pe_indices);

		Row(Row &&p_movee);

		virtual ~Row() {};

		virtual void AppendCell(const Cell &cell);

		virtual Cell &GetCell(int col);

		virtual Cell &operator[](int col);
	protected:
		std::vector<Cell> m_cells;
		int m_cellCur = 0, m_cellCount;
		int *me_indices;
	};

	// スキーマです。
	// ローの読込みを制御し、再入力されたデータを検証できる。
	class Schema
	{
	public:
		void Append(const std::u8string &p_type);

		void Clear();

		virtual void ReadRow(Row& p_row, xybase::BinaryStream &p_dataStream);

		// TODO: Implement this!
		// virtual void WriteRow(xybase::BinaryStream &p_dataStream, const Row &p_row);

	private:
		std::list<DataType> m_schema;
	};

	Schema &GetSchema();

	void AppendIndex(int idx);

	void AppendBlock(const BlockInfo &block);

	void LoadAll();

	/**
	 * @brief 载入指定行所在的整个区块（若已加载则忽略
	 * @param row 要载入的行
	 */
	void LoadRow(int row);

	void UnloadAll();

	Cell &GetCell(int row, int col);

	Row &GetRow(int row);

	Row &operator[](int row);

protected:
	std::u8string m_name;
	std::u8string m_mode;
	std::u8string m_type;
	std::u8string m_lang;
	int m_columnMax;
	int m_columnCount;
	int m_cache;

	Schema m_schema;
	int *m_indices;
	int m_indicesCur = 0;
	std::list<BlockInfo> m_blocks;
	std::map<int, Row> m_rows;
private:

	void LoadBlock(const BlockInfo &p_block);
};

