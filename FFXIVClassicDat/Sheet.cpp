#include "Sheet.h"

#include <format>
#include <cassert>
#include <iostream>

#include "DataManager.h"
#include "ShuffleString.h"
#include "BinaryData.h"
#include "xybase/Exception/InvalidParameterException.h"
#include "xybase/xystring.h"
#include "SimpleString.h"
#include "CsvUtility.h"

Sheet::Sheet(const std::u8string &name, int columnMax, int columnCount, int cache, const std::u8string &type, const std::u8string &lang)
	: m_name(name), m_columnCount(columnCount), m_columnMax(columnMax), m_cache(cache), m_type(type), m_lang(lang)
{
	m_indices = new int[m_columnCount];
}

Sheet::Sheet(Sheet &&p_movee) noexcept
	: m_name(p_movee.m_name), m_columnCount(p_movee.m_columnCount), m_columnMax(p_movee.m_columnMax), m_cache(p_movee.m_cache),
	m_type(p_movee.m_type), m_lang(p_movee.m_lang), m_schema(std::move(p_movee.m_schema)), m_indices(p_movee.m_indices),
	m_blocks(std::move(p_movee.m_blocks)), m_indicesCur(p_movee.m_indicesCur)
{
	p_movee.m_indices = nullptr;
}

Sheet::~Sheet()
{
	if(m_indices) delete[]m_indices;
}

const std::u8string &Sheet::GetName() const
{
	return m_name;
}

const std::u8string Sheet::ToCsv() const
{
	CsvGenerateUtility cgu;

	// TODO: 允许完全展开表？（使用m_columnMax
	// 表头
	cgu.AddCell(u8"type");
	for (auto &&type : m_schema.GetSchemaDefinition())
	{
		cgu.AddCell(Sheet::Schema::GetTypeName(type));
	}
	int rowCount = cgu.NewRow();
	cgu.AddCell(u8"idx");
	for (int i = 0; i < m_columnCount; ++i)
	{
		cgu.AddCell(xybase::string::itos<char8_t>(m_indices[i]));
	}
	cgu.NewRow();
	

	for (auto &&block : m_blocks)
	{
		for (auto &pair : m_rows)
		{
			cgu.AddCell(xybase::string::itos<char8_t>(pair.first));
			for (auto &&cell : pair.second.GetRawRef())
			{
				cgu.AddCell(cell.ToString());
			}
			cgu.NewRow();
		}
	}

	return cgu.ToString();
}

Sheet::Schema &Sheet::GetSchema()
{
	return m_schema;
}

void Sheet::AppendIndex(int idx)
{
	m_indices[m_indicesCur++] = idx;
}

void Sheet::AppendBlock(const BlockInfo &block)
{
	m_blocks.push_back(block);
}

void Sheet::LoadAll()
{
	for (auto &&block : m_blocks)
	{
		LoadBlock(block);
	}
}


void Sheet::Schema::Append(const std::u8string &p_type)
{
	if (p_type == u8"s8") m_schema.push_back(SDT_S8);
	else if (p_type == u8"u8") m_schema.push_back(SDT_U8);
	else if (p_type == u8"s16") m_schema.push_back(SDT_S16);
	else if (p_type == u8"u16") m_schema.push_back(SDT_U16);
	else if (p_type == u8"s32") m_schema.push_back(SDT_S32);
	else if (p_type == u8"u32") m_schema.push_back(SDT_U32);
	else if (p_type == u8"f16") m_schema.push_back(SDT_F16);
	else if (p_type == u8"float") m_schema.push_back(SDT_F32);
	else if (p_type == u8"bool") m_schema.push_back(SDT_BOOL);
	else if (p_type == u8"str") m_schema.push_back(SDT_STR);
	else throw xybase::InvalidParameterException(L"p_type", std::format(L"Unknown data type :{}", xybase::string::to_wstring(p_type)), 405390);
}

void Sheet::Schema::Clear()
{
	m_schema.clear();
}

const std::list<Sheet::DataType> &Sheet::Schema::GetSchemaDefinition() const
{
	return m_schema;
}

void Sheet::Schema::ReadRow(Row &p_row, xybase::BinaryStream &p_dataStream, size_t limit)
{
	for (DataType type : m_schema)
	{
		Cell cell(type);

		if (p_dataStream.Tell() == limit)
		{
			// TODO: 检查是否需要保持默认值
			// p_row.AppendCell(cell);
			continue;
		}

		if (type & SDT_FLAG_INTEGER)
		{
			if (type & SDT_FLAG_8BIT)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(p_dataStream.ReadInt8());
				else
					cell.Set<unsigned int>(p_dataStream.ReadUInt8());
			}
			else if (type & SDT_FLAG_16BIT)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(p_dataStream.ReadInt16());
				else
					cell.Set<unsigned int>(p_dataStream.ReadUInt16());
			}
			else if (type & SDT_FLAG_32BIT)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(p_dataStream.ReadInt32());
				else
					cell.Set<unsigned int>(p_dataStream.ReadUInt32());
			}
			else abort();
		}
		else if (type & SDT_FLAG_FLOAT)
		{
			if (type & SDT_FLAG_16BIT)
			{
				uint16_t val = p_dataStream.ReadUInt16();
				int sign = val & 0x8000;
				int exp = val & 0x3C00;
				int frac = val & 0x03FF;

				// 特殊值处理
				if (exp == 0x3C00) // exp all 1
				{
					exp = 0x7F800000;
				}
				else if (exp == 0) // exp all 0
				{
					if (frac) --frac;
					frac <<= 13;
				}
				else
					exp = ((exp + 127 - 15) << 23), frac <<= 13;

				uint32_t result = (sign << 16)
					| exp | frac;
				cell.Set<float>(*reinterpret_cast<float *>(&result));
			}
			else
				cell.Set<float>(p_dataStream.ReadFloat());
		}
		else if (type & SDT_FLAG_BOOL)
		{
			cell.Set<bool>(p_dataStream.ReadUInt8());
		}
		else if (type & SDT_FLAG_STR)
		{
			int length = p_dataStream.ReadUInt16();
			char *str = new char[length];

			p_dataStream.ReadBytes(str, length);

			SimpleString ss;
			int actualLength = ss.Decrypt(str, length, str, length);
			if (actualLength < 0) actualLength = length;
			cell.SetString((char8_t *)str);

			delete[] str;
		}
		else abort();

		p_row.AppendCell(cell);
	}
}

std::u8string Sheet::Schema::GetTypeName(DataType p_type)
{
	if (p_type == SDT_U8) return u8"u8";
	if (p_type == SDT_U16) return u8"u16";
	if (p_type == SDT_U32) return u8"u32";
	if (p_type == SDT_S8) return u8"s8";
	if (p_type == SDT_S16) return u8"s16";
	if (p_type == SDT_S32) return u8"s32";
	if (p_type == SDT_F16) return u8"f16";
	if (p_type == SDT_F32) return u8"float";
	if (p_type == SDT_STR) return u8"str";
	if (p_type == SDT_BOOL) return u8"bool";

	throw xybase::InvalidParameterException(L"p_type", L"Invalid DataType!", 59010);
}

Sheet::Cell::Cell()
	: m_type(SDT_INVALID)
{
	m_plainValue.u_val = 0;
}

Sheet::Cell::Cell(DataType p_type)
	: m_type(p_type)
{
	m_plainValue.u_val = 0;
}

Sheet::Cell::Cell(const Cell &p_pat)
	: m_type(p_pat.m_type), m_str(p_pat.m_str), m_plainValue(p_pat.m_plainValue)
{
}

Sheet::Cell::Cell(Cell &&p_movee) noexcept
	: m_str(std::move(p_movee.m_str)), m_plainValue(p_movee.m_plainValue), m_type(p_movee.m_type)
{
}

const Sheet::Cell &Sheet::Cell::operator=(const Cell &p_rval)
{
	m_str = p_rval.m_str;
	m_type = p_rval.m_type;
	m_plainValue = p_rval.m_plainValue;
	return *this;
}

inline std::u8string Sheet::Cell::ToString() const
{
	if (m_type & SDT_FLAG_INTEGER)
	{
		if (m_type & SDT_FLAG_SIGNED)
		{
			if (m_plainValue.i_val < 0)
				return u8"-" + xybase::string::itos<char8_t>(-m_plainValue.i_val);
			else
				return xybase::string::itos<char8_t>(m_plainValue.i_val);
		}
		else
			return xybase::string::itos<char8_t>(m_plainValue.u_val);
	}
	else if (m_type & SDT_FLAG_BOOL)
	{
		return m_plainValue.b_val ? u8"true" : u8"false";
	}
	else if (m_type & SDT_FLAG_FLOAT)
	{
		return (char8_t *)std::to_string(m_plainValue.f_val).c_str();
	}
	else if (m_type & SDT_FLAG_STR)
	{
		return m_str;
	}
	else if (m_type == SDT_INVALID)
	{
		return u8"";
	}
}

Sheet::Row::Row(int columnCount, int *pe_indices)
	: m_cells(columnCount), me_indices(pe_indices), m_cellCount(columnCount)
{
}

Sheet::Row::Row(Row &&p_movee) noexcept
	: m_cells(std::move(p_movee.m_cells)), me_indices(p_movee.me_indices), m_cellCount(p_movee.m_cellCount)
{
}

void Sheet::Row::AppendCell(const Cell &cell)
{
	m_cells[m_cellCur++] = cell;
}

Sheet::Cell &Sheet::Row::GetCell(int col)
{
	for (int i = 0; i < m_cellCur; ++i)
	{
		if (col == me_indices[i])
		{
			return m_cells[i];
		}
	}
	throw xybase::InvalidParameterException(L"col", L"Specified col has not been defined.", 7520);
}

Sheet::Cell &Sheet::Row::operator[](int col)
{
	return GetCell(col);
}

const std::vector<Sheet::Cell> &Sheet::Row::GetRawRef() const
{
	return m_cells;
}

void Sheet::LoadRow(int row)
{
	if (m_rows.contains(row)) return;
	for (auto &&block : m_blocks)
	{
		if (block.begin <= row && block.begin + block.count > row)
			LoadBlock(block);
	}
}

void Sheet::UnloadAll()
{
	m_blocks.clear();
}

Sheet::Cell &Sheet::GetCell(int row, int col)
{
	return GetRow(row).GetCell(col);
}

Sheet::Row &Sheet::GetRow(int row)
{
	auto target = m_rows.find(row);
	// Cache Miss!
	if (target == m_rows.end())
	{
		LoadRow(row);
		return m_rows.find(row)->second;
	}
	else
		return target->second;
}

Sheet::Row &Sheet::operator[](int row)
{
	return GetRow(row);
}

inline uint32_t GetBeginingOffset(uint32_t *offset, int idx, size_t length)
{
	assert(idx < length);
	return idx == 0 ? 0 : offset[idx - 1];
	// return offset[idx];
}

void Sheet::LoadBlock(const BlockInfo &p_block)
{
	BinaryData offset = DataManager::GetInstance().LoadData(p_block.offset);
	BinaryData enable = DataManager::GetInstance().LoadData(p_block.enable);
	// BinaryData data = DataManager::GetInstance().LoadData(p_block.data);
	xybase::BinaryStream *data = DataManager::GetInstance().NewDataStream(p_block.data, L"rb");
	
	assert(!(offset.GetLength() & 0x3));
	assert(!(enable.GetLength() & 0x7));

	struct EnableEntry { uint32_t idx; uint32_t cnt; } *enableEntries = (EnableEntry *)enable.GetData();
	int enableEntriesCount = enable.GetLength() / 8;
	uint32_t *offsets = (uint32_t *)offset.GetData();
	int offsetCount = offset.GetLength() / 4;
	// assert(p_block.count == offsetCount);

	if (m_cfgIgnoreEnableIndication)
	{
		for (int i = 0; i < p_block.count; ++i)
		{
			data->Seek(GetBeginingOffset(offsets, i, offsetCount));
			Sheet::Row row(m_columnCount, m_indices);
			m_schema.ReadRow(row, *data, offsets[i]);
			m_rows.insert(std::make_pair(i, std::move(row)));
			assert(data->Tell() == offsets[i]);
		}
	}
	else
	{
		for (int i = 0; i < enableEntriesCount; ++i)
		{
			for (int j = 0; j < enableEntries[i].cnt; ++j)
			{
				int idx = enableEntries[i].idx + j;
				data->Seek(GetBeginingOffset(offsets, idx - p_block.begin, offsetCount));
				Sheet::Row row(m_columnCount, m_indices);
				try
				{
					m_schema.ReadRow(row, *data, offsets[idx - p_block.begin]);
				}
				catch (xybase::IOException &ex)
				{
					if (data->IsEof() && data->Tell() == offsets[idx - p_block.begin])
					{
						std::cerr << std::format("Warning! data[{:08X}]:row {} wield! early cut!\n", p_block.data, idx);
					}
					else
					{
						throw xybase::RuntimeException(
							std::format(
								L"Ill-formed sheet row! data={:08X}, enable={:08X}, offset={:08X}, row={}",
								p_block.data, p_block.enable, p_block.offset, idx), 717010);
					}
				}
				m_rows.insert(std::make_pair(idx, std::move(row)));
				assert(data->Tell() == offsets[idx - p_block.begin]);
			}
		}
	}

	delete data;
}
