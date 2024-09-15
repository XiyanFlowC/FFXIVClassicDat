#include "CsvUtility.h"

#include "xybase/xystring.h"

CsvFile::CsvFile(std::wstring filePath, OperationType mode)
{
	m_stream = new xybase::BinaryStream(filePath, mode == OperationType::Append ? L"a" : (mode == OperationType::Read ? L"r" : L"w"));
	if (mode == OperationType::Read)
	{
		m_stream->Seek(0, xybase::Stream::SM_END);
		m_size = m_stream->Tell();
		m_stream->Seek(0);

		char buf[3];
		m_stream->ReadBytes(buf, 3);
		// 不为 BOM，Rewind
		if (memcmp(buf, "\xEF\xBB\xBF", 3)) m_stream->Seek(0);
	}
	if (mode == OperationType::Write)
	{
		m_stream->Write("\xEF\xBB\xBF", 3);
	}
}

CsvFile::~CsvFile()
{
	delete m_stream;
}

std::u8string CsvFile::NextCell()
{
	if (m_eolFlag) throw xybase::InvalidOperationException(L"No more cell!", 45001);

	xybase::StringBuilder<char8_t> sb;

	int x = m_stream->Tell();

	char8_t ch = m_stream->ReadUInt8();
	if (ch == '\"')
	{
		ch = m_stream->ReadUInt8();
		while ((1))
		{
			if (ch == '\"')
			{
				ch = m_stream->ReadUInt8();
				if (ch == '\"')
					sb.Append('\"');
				else if (ch == ',')
					break;
				else if (ch == '\n')
					break;
				else
				{
					throw xybase::RuntimeException(
						std::format(L"读取的CSV文件格式不正。字符位置{}处。", m_stream->Tell()),
						45002);
				}
			}
			else sb.Append(ch);

			ch = m_stream->ReadUInt8();
		}
		printf("%llX\n", m_stream->Tell());
	}
	else while (ch != ',' && ch != '\n')
	{
		sb.Append(ch);

		ch = m_stream->ReadUInt8();
	}

	m_eolFlag = ch == '\n';

	return sb.ToString();
}

void CsvFile::NewCell(const std::u8string &p_str)
{
	if (m_firstCellFlag)
		m_firstCellFlag = false;
	else
		m_stream->Write((uint8_t)',');

	if (p_str.find_first_of(u8"\n\r\",") != std::u8string::npos)
	{
		m_stream->Write((uint8_t)'"');
		for (auto &&ch : p_str)
		{
			if (ch == '"') m_stream->Write("\"\"", 2);
			else m_stream->Write((uint8_t) ch);
		}
		m_stream->Write((uint8_t)'"');
	}
	else
	{
		m_stream->Write((char *)p_str.c_str(), p_str.size());
	}
}

bool CsvFile::IsEol()
{
	return m_eolFlag;
}

void CsvFile::NextLine()
{
	if (!m_eolFlag)
	{
		int ch = m_stream->ReadUInt8();
		while (ch != '\n') ch = m_stream->ReadUInt8();
	}
	m_eolFlag = false;
}

void CsvFile::NewLine()
{
	m_firstCellFlag = true;
	m_stream->Write((uint8_t)'\n');
}

bool CsvFile::IsEof()
{
	if (m_size) return m_stream->Tell() >= m_size;
	return m_stream->IsEof();
}

void CsvFile::Close()
{
	m_stream->Close();
}
