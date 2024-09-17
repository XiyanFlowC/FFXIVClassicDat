#include "GameStringUtil.h"

#include <format>
#include <string>
#include <cstdio>
#include <cstring>
#include "xybase/StringBuilder.h"


std::u8string GameStringUtil::Decode(std::u8string_view p_str)
{
	m_str = p_str;
	m_pos = 0;
	xybase::StringBuilder<char8_t> sb;
	const char8_t buf[4] = { 0 };
	while (m_pos < p_str.length())
	{
		if (p_str[m_pos] == CONTROL_SEQ_START)
		{
			int tag = p_str[++m_pos];
			++m_pos;
			sb += ProcessTag(tag);
		}
		else
		{
			char8_t ch = p_str[m_pos++];
			if (ch == '\\')
				sb += u8"\\\\";
			else if (ch == '<')
				sb += u8"\\<";
			else if (ch == '>')
				sb += u8"\\>";
			else if (ch < 0x32 || ch == 0x7F)
			{
				sprintf((char *)buf, "\\%02X", ch);
				sb += buf;
			}
			else
				sb += p_str[ch];
		}
	}

	return sb.ToString();
}

std::u8string GameStringUtil::ProcessTag(const char8_t tag)
{
	xybase::StringBuilder sb;



	return (char8_t *)sb.ToString();
}
