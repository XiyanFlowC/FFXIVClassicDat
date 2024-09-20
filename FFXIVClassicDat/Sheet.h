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

class CsvFile;

/**
 * @brief Sheet 是存储在data中的一类dat。这里是实际Sheet数据（enable offset 和 data）的解析和保存。
 * Sheet由Ssd文件所定义。
 */
class Sheet
{
public:
	bool m_cfgIgnoreEnableIndication = false;

	bool m_cfgInputVerifySchema = true;
	bool m_cfgInputVerifyIndex = false;

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

	Sheet(const std::u8string& name, int columnMax, int columnCount, int cache, const std::u8string &type, const std::u8string &lang, const std::u8string &param);

	Sheet(const Sheet &) = delete;

	Sheet(Sheet && p_movee) noexcept;

	~Sheet();

	const std::u8string &GetName() const;

	/**
	 * @brief 将已经载入的数据写入给定的Csv流
	 * @param p_csv 
	 */
	void SaveToCsv(CsvFile &p_csv) const;

	/**
	 * @brief 从给定的Csv流中读取数据
	 * @param p_csv 
	 */
	void LoadFromCsv(CsvFile &p_csv);

	/**
	 * @brief 读取所有区块
	 */
	void LoadAll();

	/**
	 * @brief 载入指定行所在的整个区块（若已加载则忽略
	 * @param row 要载入的行
	 */
	void LoadRow(int row);

	/**
	 * @brief 卸载所有加载的区块。
	 */
	void UnloadAll();

	/**
	 * @brief 保存所有区块。未被加载的区块或行将被置空。
	 */
	void SaveAll();

	/**
	 * @brief 保存此表的Enable项到Csv文件中
	 * @param p_csv 
	 */
	void EnableToCsv(CsvFile &p_csv) const;

	/**
	 * @brief 从Csv文件中读取此表的Enable
	 * @param p_csv 
	 */
	void EnableFromCsv(CsvFile &p_csv);

	/* 成员类定义 */

	/**
	 * @brief 块数据。索引元数据。每个Sheet由若干个 file 保存。这里记录每个 block/file 的关联信息。
	 */
	struct BlockInfo
	{
		// 启用提示文件编号
		uint32_t enable;
		// 偏移提示文件编号
		uint32_t offset;
		// 数据文件编号
		uint32_t data;
		// 该块的起始行号
		int begin;
		// 该块中包含的行数
		int count;
	};

	/**
	 * @brief 单元格数据。实体。
	 */
	class Cell
	{
		void CheckDataStatus(DataType flags) const
		{
			if ((flags & SDT_MASK_TYPE) ^ (m_type & SDT_MASK_TYPE))
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
		T Get() const;

		template<>
		int Get() const
		{
			CheckDataStatus(SDT_FLAG_INTEGER);
			return m_plainValue.i_val;
		}

		template<>
		unsigned int Get() const
		{
			CheckDataStatus(SDT_FLAG_INTEGER);
			return m_plainValue.u_val;
		}

		template<>
		float Get() const
		{
			CheckDataStatus(SDT_FLAG_FLOAT);
			return m_plainValue.f_val;
		}

		template<>
		bool Get() const
		{
			CheckDataStatus(SDT_FLAG_BOOL);
			return m_plainValue.b_val;
		}

		template<>
		std::string Get() const
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
		void Set(std::string_view value)
		{
			CheckDataStatus(SDT_FLAG_STR);
			m_str = value;
		}

		void SetString(std::string_view value)
		{
			CheckDataStatus(SDT_FLAG_STR);
			m_str = value;
		}

		std::string ToString() const;

		DataType GetType() const;
	protected:
		DataType m_type;
		union
		{
			int i_val;
			unsigned int u_val;
			float f_val;
			bool b_val;
		} m_plainValue;

		std::string m_str;
	};

	/**
	 * @brief 行数据。实体。表数据读取到内存中用此组织。
	 */
	class Row
	{
	public:
		Row(int columnCount, int* pe_indices);

		Row(Row &&p_movee) noexcept;

		virtual ~Row() {};

		virtual void AppendCell(const Cell &cell);

		virtual Cell &GetCell(int col);

		virtual Cell &operator[](int col);

		const std::vector<Cell> &GetRawRef() const;

		const std::wstring ToString() const;
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

		const std::list<DataType> &GetSchemaDefinition() const;

		/**
		 * @brief 读取一行的数据
		 * @param p_row 数据要保存到的行对象
		 * @param p_dataStream 数据流
		 * @param limit 最大流位置：一个数据记录可能提早结束。
		 */
		virtual void ReadRow(Row& p_row, xybase::BinaryStream &p_dataStream, size_t limit);

		/**
		 * @brief 写入一行的数据
		 * @param p_row 数据来源的对象
		 * @param p_dataStream 要写入的数据流
		 * @param p_offsetStream 要写入偏移的数据流
		 */
		virtual void WriteRow(const Row &p_row, xybase::BinaryStream &p_dataStream, xybase::BinaryStream &p_offsetStream);

		// TODO: Implement this!
		// virtual void WriteRow(xybase::BinaryStream &p_dataStream, const Row &p_row);

		static std::u8string GetTypeName(DataType p_type);

	private:
		std::list<DataType> m_schema;
	};

	/**
	 * @brief 初始化时用，添加索引元数据
	 * @param idx
	 */
	void AppendIndex(int idx);

	/**
	 * @brief 初始化时用，添加区块元数据
	 * @param block
	 */
	void AppendBlock(const BlockInfo &block);

	class Schema &GetSchema();

	class Cell &GetCell(int row, int col);

	class Row &GetRow(int row);

	Row &operator[](int row);

	std::wstring ToString() const;

protected:
	std::u8string m_param;
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
	void SaveBlock(const BlockInfo &p_block);
};

