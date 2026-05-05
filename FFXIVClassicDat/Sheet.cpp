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
#include "GameStringUtil.h"

Sheet::Sheet(const std::u8string &name, int columnMax, int columnCount, int cache, const std::u8string &type, const std::u8string &lang, const std::u8string &param)
	: m_name(name), m_columnCount(columnCount), m_columnMax(columnMax), m_cache(cache), m_type(type), m_lang(lang), m_param(param)
{
	m_indices = new int[m_columnCount];
}

Sheet::Sheet(Sheet &&movee) noexcept
	: m_name(movee.m_name), m_columnCount(movee.m_columnCount), m_columnMax(movee.m_columnMax), m_cache(movee.m_cache),
	m_type(movee.m_type), m_lang(movee.m_lang), m_schema(std::move(movee.m_schema)), m_indices(movee.m_indices),
	m_blocks(std::move(movee.m_blocks)), m_indicesCur(movee.m_indicesCur), m_param(std::move(movee.m_param))
{
	movee.m_indices = nullptr;
}

Sheet::~Sheet()
{
	if(m_indices) delete[]m_indices;
}

const std::u8string &Sheet::GetName() const
{
	return m_name;
}

void Sheet::SaveToCsv(CsvFile &csv) const
{
	csv.NewCell(u8"type");
	for (auto &&type : m_schema.GetSchemaDefinition())
	{
		csv.NewCell(Sheet::Schema::GetTypeName(type));
	}
	csv.NewLine();
	csv.NewCell(u8"idx");
	for (int i = 0; i < m_columnCount; ++i)
	{
		csv.NewCell(xybase::string::itos<char8_t>(m_indices[i]));
	}
	csv.NewLine();

	GameStringUtil gs;
	for (auto &pair : m_rows)
	{
		csv.NewCell(xybase::string::itos<char8_t>(pair.first));
		for (auto &&cell : pair.second.GetRawRef())
		{
			if (cell.GetType() == SDT_INVALID) break;
			if (cell.GetType() == SDT_STR)
				csv.NewCell(gs.Decode(cell.ToString()));
			else
				csv.NewCell((char8_t *)cell.ToString().c_str());
		}
		csv.NewLine();
	}
}

void Sheet::LoadFromCsv(CsvFile &csv)
{
	if (csv.NextCell() != u8"type") throw xybase::InvalidParameterException(L"csv", L"Invalid csv.", 54801);
	// 验证数据约定
	if (m_cfgInputVerifySchema)
		for (auto &&type : m_schema.GetSchemaDefinition())
		{
			if (csv.NextCell() != Sheet::Schema::GetTypeName(type)) throw xybase::InvalidParameterException(L"csv", L"Schema mismatch!", 54802);
		}
	csv.NextLine();
	if (csv.NextCell() != u8"idx") throw xybase::InvalidParameterException(L"csv", L"Invalid csv.", 54803);
	// 验证索引
	if (m_cfgInputVerifyIndex)
		for (int i = 0; i < m_columnCount; ++i)
		{
			if (csv.NextCell() != xybase::string::itos<char8_t>(m_indices[i])) throw xybase::InvalidParameterException(L"csv", L"Index mismatch!", 54804);;
		}
	csv.NextLine();

	GameStringUtil gsu;
	while (!csv.IsEof())
	{
		Row row(m_columnCount, m_indices);
		int rowId = xybase::string::stoi<char8_t>(csv.NextCell());
		for (DataType type : m_schema.GetSchemaDefinition())
		{
			if (csv.IsEol()) break;
			Cell cell(type);
			if (type & SDT_FLAG_INTEGER)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(xybase::string::pint<char8_t>(csv.NextCell()));
				else
					cell.Set<unsigned int>(xybase::string::pint<char8_t>(csv.NextCell()));
			}
			else if (type & SDT_FLAG_FLOAT)
			{
				cell.Set<float>(xybase::string::pflt(csv.NextCell()));
			}
			else if (type & SDT_FLAG_BOOL)
			{
				auto str = xybase::string::to_lower(csv.NextCell());
				if (str != u8"true" && str != u8"false")
					throw xybase::InvalidParameterException(L"csv", 
						std::format(L"Bool type parse failed. (For row {})", rowId), 54805);
				cell.Set<bool>(str == u8"true");
			}
			else if (type & SDT_FLAG_STR)
			{
				cell.SetString(gsu.Encode(csv.NextCell().c_str()));
			}
				else abort();
				row.AppendCell(cell);
			}
		// 為確保從CSV中取得的數據覆蓋舊有的的數據。
			auto exists = m_rows.find(rowId);
		if (exists != m_rows.end()) m_rows.erase(exists);
		m_rows.insert(std::make_pair(rowId, std::move(row)));
		csv.NextLine();
	}
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

#include <iostream>

void Sheet::LoadAll()
{
	for (auto &&block : m_blocks)
	{
		try
		{
			LoadBlock(block);
		}
		catch (DataManager::FileMissingException &ex)
		{
			/* Rescue: log and skip */
			std::wcout << L"读取错误。" << ToString() << L"中的块" << block.begin << "[" << block.count << L"]无法完成读取。";
			std::wcout << std::format(L"文件缺失：{:08X}", ex.GetFileId());
			std::wcout << L"此错误将被忽略。\n";
		}
	}
}


void Sheet::Schema::Append(const std::u8string &type)
{
	if (type == u8"s8") m_schema.push_back(SDT_S8);
	else if (type == u8"u8") m_schema.push_back(SDT_U8);
	else if (type == u8"s16") m_schema.push_back(SDT_S16);
	else if (type == u8"u16") m_schema.push_back(SDT_U16);
	else if (type == u8"s32") m_schema.push_back(SDT_S32);
	else if (type == u8"u32") m_schema.push_back(SDT_U32);
	else if (type == u8"f16") m_schema.push_back(SDT_F16);
	else if (type == u8"float") m_schema.push_back(SDT_F32);
	else if (type == u8"bool") m_schema.push_back(SDT_BOOL);
	else if (type == u8"str") m_schema.push_back(SDT_STR);
	else throw xybase::InvalidParameterException(L"type", std::format(L"Unknown data type :{}", xybase::string::to_wstring(type)), 405390);
}

void Sheet::Schema::Clear()
{
	m_schema.clear();
}

const std::list<Sheet::DataType> &Sheet::Schema::GetSchemaDefinition() const
{
	return m_schema;
}

void Sheet::Schema::ReadRow(Row &row, xybase::BinaryStream &dataStream, size_t limit)
{
	for (DataType type : m_schema)
	{
		Cell cell(type);

		if (dataStream.Tell() == limit)
		{
			/* TODO: Check whether to keep default value */
			continue;
		}

		if (type & SDT_FLAG_INTEGER)
		{
			if (type & SDT_FLAG_8BIT)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(dataStream.ReadInt8());
				else
					cell.Set<unsigned int>(dataStream.ReadUInt8());
			}
			else if (type & SDT_FLAG_16BIT)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(dataStream.ReadInt16());
				else
					cell.Set<unsigned int>(dataStream.ReadUInt16());
			}
			else if (type & SDT_FLAG_32BIT)
			{
				if (type & SDT_FLAG_SIGNED)
					cell.Set<int>(dataStream.ReadInt32());
				else
					cell.Set<unsigned int>(dataStream.ReadUInt32());
			}
			else abort();
		}
		else if (type & SDT_FLAG_FLOAT)
		{
			if (type & SDT_FLAG_16BIT)
			{
				uint16_t val = dataStream.ReadUInt16();
				int sign = val & 0x8000;
				int rest = val & ~0x8000;

				uint32_t result = (sign << 16) | ((rest << 13) + 0x38000000);
				cell.Set<float>(*reinterpret_cast<float *>(&result));
			}
			else
				cell.Set<float>(dataStream.ReadFloat());
		}
		else if (type & SDT_FLAG_BOOL)
		{
			cell.Set<bool>(dataStream.ReadUInt8());
		}
		else if (type & SDT_FLAG_STR)
		{
			int length = dataStream.ReadUInt16();
			char *str = new char[length];

			dataStream.ReadBytes(str, length);

			SimpleString ss;
			int actualLength = ss.Decrypt(str, length, str, length);
			if (actualLength < 0) actualLength = length;
			cell.SetString(str);

			delete[] str;
		}
		else abort();

		row.AppendCell(cell);
	}
}

void Sheet::Schema::WriteRow(const Row &row, xybase::BinaryStream &dataStream, xybase::BinaryStream &offsetStream)
{
	auto formalTypeItr = m_schema.begin();
	for (const Cell &cell : row.GetRawRef())
	{
		DataType formalType = *formalTypeItr++;

		if (cell.GetType() == SDT_INVALID)
		{
			break;
		}
		if ((formalType & SDT_MASK_TYPE) != (cell.GetType() & SDT_MASK_TYPE))
			throw xybase::InvalidParameterException(L"row", L"Row data violates schema!", 99010);

		if (formalType & SDT_FLAG_INTEGER)
		{
			if (formalType & SDT_FLAG_8BIT)
			{
				if (formalType & SDT_FLAG_SIGNED)
					dataStream.Write((int8_t) cell.Get<int>());
				else
					dataStream.Write((uint8_t)cell.Get<unsigned int>());
			}
			else if (formalType & SDT_FLAG_16BIT)
			{
				if (formalType & SDT_FLAG_SIGNED)
					dataStream.Write((int16_t)cell.Get<int>());
				else
					dataStream.Write((uint16_t)cell.Get<unsigned int>());
			}
			else if (formalType & SDT_FLAG_32BIT)
			{
				if (formalType & SDT_FLAG_SIGNED)
					dataStream.Write((int32_t)cell.Get<int>());
				else
					dataStream.Write((uint32_t)cell.Get<unsigned int>());
			}
			else abort();
		}
		else if (formalType & SDT_FLAG_FLOAT)
		{
			if (formalType & SDT_FLAG_16BIT)
			{
				float value = cell.Get<float>();
				uint32_t val = *reinterpret_cast<uint32_t *>(&value);
				unsigned sign = val & 0x80000000;
				unsigned rest = val & ~0x80000000;
				sign >>= 16;
				rest -= 0x38000000;
				rest >>= 13;
				val = sign | rest;
				dataStream.Write((uint16_t)val);
			}
			else
				dataStream.Write(cell.Get<float>());
		}
		else if (formalType & SDT_FLAG_BOOL)
		{
			dataStream.Write((uint8_t) cell.Get<bool>());
		}
		else if (formalType & SDT_FLAG_STR)
		{
			auto str = cell.Get<std::string>();
			int length = str.length() + 1;
			dataStream.Write((uint16_t)length);
			dataStream.Write((char *)str.c_str(), length);
		}
		else abort();
	}
	offsetStream.Write((uint32_t)dataStream.Tell());
}

std::u8string Sheet::Schema::GetTypeName(DataType type)
{
	if (type == SDT_U8) return u8"u8";
	if (type == SDT_U16) return u8"u16";
	if (type == SDT_U32) return u8"u32";
	if (type == SDT_S8) return u8"s8";
	if (type == SDT_S16) return u8"s16";
	if (type == SDT_S32) return u8"s32";
	if (type == SDT_F16) return u8"f16";
	if (type == SDT_F32) return u8"float";
	if (type == SDT_STR) return u8"str";
	if (type == SDT_BOOL) return u8"bool";

	throw xybase::InvalidParameterException(L"type", L"Invalid DataType!", 59010);
}

Sheet::Cell::Cell()
	: m_type(SDT_INVALID)
{
	m_plainValue.u_val = 0;
}

Sheet::Cell::Cell(DataType type)
	: m_type(type)
{
	m_plainValue.u_val = 0;
}

Sheet::Cell::Cell(const Cell &other)
	: m_type(other.m_type), m_str(other.m_str), m_plainValue(other.m_plainValue)
{
}

Sheet::Cell::Cell(Cell &&movee) noexcept
	: m_str(std::move(movee.m_str)), m_plainValue(movee.m_plainValue), m_type(movee.m_type)
{
}

const Sheet::Cell &Sheet::Cell::operator=(const Cell &other)
{
	m_str = other.m_str;
	m_type = other.m_type;
	m_plainValue = other.m_plainValue;
	return *this;
}

inline std::string Sheet::Cell::ToString() const
{
	if (m_type & SDT_FLAG_INTEGER)
	{
		if (m_type & SDT_FLAG_SIGNED)
		{
			if (m_plainValue.i_val < 0)
				return "-" + xybase::string::itos<char>(-m_plainValue.i_val);
			else
				return xybase::string::itos<char>(m_plainValue.i_val);
		}
		else
			return xybase::string::itos<char>(m_plainValue.u_val);
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
		return m_str;
	}
	else if (m_type == SDT_INVALID)
	{
		return "";
	}
}

Sheet::DataType Sheet::Cell::GetType() const
{
	return m_type;
}

Sheet::Row::Row(int columnCount, int *indices)
	: m_cells(columnCount), m_indices(indices), m_cellCount(columnCount)
{
}

Sheet::Row::Row(Row &&movee) noexcept
	: m_cells(std::move(movee.m_cells)), m_indices(movee.m_indices), m_cellCount(movee.m_cellCount)
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
		if (col == m_indices[i])
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

const std::wstring Sheet::Row::ToString() const
{
	std::wstringstream wss;
	wss << L"<Row ";
	for (auto &&cell : m_cells)
	{
		wss << xybase::string::to_wstring(cell.ToString()) << ',';
	}
	wss << L">";
	return wss.str();
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

void Sheet::SaveAll()
{
	for (auto &&block : m_blocks)
	{
		SaveBlock(block);
	}
}

void Sheet::EnableToCsv(CsvFile &csv) const
{
	for (auto &&block : m_blocks)
	{
		csv.NewCell(u8"[block]");
		csv.NewCell(xybase::string::itos<char8_t>(block.begin));
		csv.NewLine();
		
		try
		{
			auto enableRaw = DataManager::GetInstance().LoadData(block.enable);
			struct EnableEntry { uint32_t index; uint32_t count; };
			EnableEntry *enable = (EnableEntry *)enableRaw.GetData();
			assert(!(enableRaw.GetLength() & 0x7));
			int count = enableRaw.GetLength() / 8;

			for (int i = 0; i < count; ++i)
			{
				csv.NewCell(xybase::string::itos<char8_t>(enable[i].index));
				csv.NewCell(xybase::string::itos<char8_t>(enable[i].count));
				csv.NewLine();
			}
		}
		catch (DataManager::FileMissingException &ex)
		{
			std::wcerr << L"处理" << ToString() << "的启用信息发生异常：" << std::endl;
			std::wcerr << std::format(L"块{}[{}]指定的启用信息{:08X}不存在。\n", block.begin, block.count, block.enable);
		}
	}
}

void Sheet::EnableFromCsv(CsvFile &csv)
{
	xybase::BinaryStream *enableStream = nullptr;
	while (!csv.IsEof())
	{
		std::u8string first = csv.NextCell();
		if (first == u8"[block]")
		{
			if (enableStream) delete enableStream;
			enableStream = nullptr;
			int begin = xybase::string::stoi<char8_t>(csv.NextCell());
			for (const BlockInfo &info : m_blocks)
			{
				if (info.begin == begin)
				{
					enableStream = DataManager::GetInstance().NewDataStream(info.enable, L"wb");
				}
			}
			if (enableStream == nullptr)
				throw xybase::InvalidParameterException(L"csv", L"Not an enable file for this sheet!", 53648);
			csv.NextLine();
			continue;
		}
		
		uint32_t begin = xybase::string::stoi<char8_t>(first);
		uint32_t count = xybase::string::stoi<char8_t>(csv.NextCell());
		enableStream->Write(begin);
		enableStream->Write(count);
	}
	if (enableStream) delete enableStream;
}

Sheet::Cell &Sheet::GetCell(int row, int col)
{
	return GetRow(row).GetCell(col);
}

Sheet::Row &Sheet::GetRow(int row)
{
	auto target = m_rows.find(row);
	if (target == m_rows.end())
	{
		LoadRow(row);
		if (!m_rows.contains(row))
		{
			Row ret(m_columnCount, m_indices);
			m_rows.insert(std::make_pair(row, std::move(ret)));
		}
		return m_rows.find(row)->second;
	}
	else
		return target->second;
}

Sheet::Row &Sheet::operator[](int row)
{
	return GetRow(row);
}

std::wstring Sheet::ToString() const
{
	std::wstringstream wss;

	wss << L"<Sheet ";
	wss << L"Name: " << xybase::string::to_wstring(m_name) << L"\n";
	wss << L"Mode: " << xybase::string::to_wstring(m_mode) << L"\n";
	wss << L"Type: " << xybase::string::to_wstring(m_type) << L"\n";
	wss << L"Lang: " << xybase::string::to_wstring(m_lang) << L"\n";
	wss << L"Param: " << xybase::string::to_wstring(m_param) << L"\n";
	wss << L"Column Max: " << m_columnMax << L"\n";
	wss << L"Column Count: " << m_columnCount << L"\n";
	wss << L"Cache: " << m_cache << L">\n";

	return wss.str();
}

inline uint32_t getBeginningOffset_(uint32_t *offset, int idx, size_t length)
{
	if (idx >= length) throw xybase::InvalidParameterException(L"idx", L"Index out of range!", 58010);
	return idx == 0 ? 0 : offset[idx - 1];
}

void Sheet::LoadBlock(const BlockInfo &block)
{
	BinaryData offset = DataManager::GetInstance().LoadData(block.offset);
	BinaryData enable = DataManager::GetInstance().LoadData(block.enable);
	xybase::BinaryStream *data = DataManager::GetInstance().NewDataStream(block.data, L"rb");
	
	assert(!(offset.GetLength() & 0x3));
	assert(!(enable.GetLength() & 0x7));

	struct EnableEntry { uint32_t idx; uint32_t cnt; } *enableEntries = (EnableEntry *)enable.GetData();
	int enableEntriesCount = enable.GetLength() / 8;
	uint32_t *offsets = (uint32_t *)offset.GetData();
	int offsetCount = offset.GetLength() / 4;

	if (m_cfgIgnoreEnableIndication)
	{
		for (int i = 0; i < block.count; ++i)
		{
			if (i >= offsetCount) break;
			assert(getBeginningOffset_(offsets, i, offsetCount) == data->Tell());
			if (offsets[i] == data->Tell()) continue;
			Sheet::Row row(m_columnCount, m_indices);
			m_schema.ReadRow(row, *data, offsets[i]);
			m_rows.insert(std::make_pair(block.begin + i, std::move(row)));
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
				
				if (idx - block.begin >= offsetCount) break;
				data->Seek(getBeginningOffset_(offsets, idx - block.begin, offsetCount));
				Sheet::Row row(m_columnCount, m_indices);
				try
				{
					m_schema.ReadRow(row, *data, offsets[idx - block.begin]);
				}
				catch (xybase::IOException &ex)
				{
					throw xybase::RuntimeException(
						std::format(
							L"Ill-formed sheet row! data={:08X}, enable={:08X}, offset={:08X}, row={}",
							block.data, block.enable, block.offset, idx), 717010);
				}
				m_rows.insert(std::make_pair(idx, std::move(row)));
				assert(data->Tell() == offsets[idx - block.begin]);
			}
		}
	}

	delete data;
}

void Sheet::SaveBlock(const BlockInfo &block)
{
	xybase::BinaryStream *offsetStream = DataManager::GetInstance().NewDataStream(block.offset, L"wb");
	xybase::BinaryStream *dataStream = DataManager::GetInstance().NewDataStream(block.data, L"wb");

	xybase::BinaryStream *enableStream = nullptr;
	/* Regenerate enable stream */
	enableStream = DataManager::GetInstance().NewDataStream(block.enable, L"wb");

	uint32_t continousCounter = 0, begin = block.begin;
	int max = block.count;
	for (; max > 0; --max)
	{
		if (m_rows.find(block.begin + max) != m_rows.end())
		{
			++max;
			break;
		}
	}

	for (int i = 0; i < max; ++i)
	{
		auto rowItr = m_rows.find(block.begin + i);
		if (rowItr == m_rows.end())
		{
			offsetStream->Write((uint32_t)dataStream->Tell());

			if (enableStream && continousCounter)
			{
				enableStream->Write(begin);
				enableStream->Write(continousCounter);
			}
			begin = block.begin + i + 1;
			continousCounter = 0;
			continue;
		}
		continousCounter++;
		m_schema.WriteRow(rowItr->second, *dataStream, *offsetStream);
	}

	if (enableStream && continousCounter)
	{
		enableStream->Write(begin);
		enableStream->Write(continousCounter);
	}

	delete offsetStream;
	delete dataStream;
	if (enableStream)
		delete enableStream;
}
