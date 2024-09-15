#pragma once

#include <filesystem>

#include "xybase/BinaryStream.h"
#include "xybase/StringBuilder.h"

class CsvFile
{
public:
	enum class OperationType
	{
		Read,
		Write,
		Append,
	};

	CsvFile(std::wstring filePath, OperationType mode);

	~CsvFile();

	std::u8string NextCell();

	void NewCell(const std::u8string &p_str);

	void NextLine();

	void NewLine();

	bool IsEol();

	bool IsEof();

	void Close();

private:
	xybase::BinaryStream *m_stream;
	size_t m_size = 0;
	bool m_eolFlag = false;
	bool m_firstCellFlag = true;
};
