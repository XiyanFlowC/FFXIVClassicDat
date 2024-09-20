#include "GameStringUtil.h"

#include <iostream>
#include <format>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "xybase/xystring.h"

GameStringUtil::TagDefinition GameStringUtil::defs[] =
{
	/*{u8"If", &GameStringUtil::DecodeTagIf, nullptr, GameStringUtil::Tag::If},
	{u8"LF", nullptr, nullptr, GameStringUtil::Tag::LineBreak},
	{nullptr, nullptr, nullptr, 0, 0, 0, GameStringUtil::Tag::None},*/
	{u8"If", 2, 3, GameStringUtil::Tag::If},
	{u8"LF", 0, 0, GameStringUtil::Tag::LineFeed},
	{u8"Value", 1, 1, GameStringUtil::Tag::Value},
	{u8"Highlight", 1, 1, Tag::Highlight},
	{u8"Switch", 1, 255, Tag::Switch},
	{u8"Sheet", 2, 255, Tag::Sheet},
	{u8"Color", 1, 255, Tag::Color},
	{u8"Color2", 1, 255, Tag::Color2},
	{u8"Icon", 1, 255, Tag::Icon},
	{u8"Format", 1, 255, Tag::Format},
	{u8"Clickable", 1, 255, Tag::Clickable},
	{u8"Split", 1, 255, Tag::Split},
	{u8"Emphasis", 1, 255, Tag::Emphasis},
	{u8"Time", 1, 255, Tag::Time},
	{u8"Indent", 1, 255, Tag::Indent},
	{u8"Dash", 1, 255, Tag::Dash},
	{u8"TwoDigitValue", 1, 255, Tag::TwoDigitValue},
	{nullptr, 0, 0, GameStringUtil::Tag::None},
};

std::u8string GameStringUtil::Decode(std::u8string_view p_str)
{
	m_str = p_str;
	m_pos = 0;
	m_sb.Clear();
	const char8_t buf[8] = { 0 };
	while (m_pos < p_str.length())
	{
		if (p_str[m_pos] == CONTROL_SEQ_START)
		{
			int tag = p_str[++m_pos];
			++m_pos;
			DecodeTag(tag);
		}
		else
		{
			char8_t ch = p_str[m_pos++];
			if (ch == '\\')
				m_sb += u8"\\\\";
			else if (ch == '<')
				m_sb += u8"\\<";
			else if (ch == '>')
				m_sb += u8"\\>";
			/*else if (ch == ',')
				m_sb += u8"\\,";*/
			else if (ch == '"')
				m_sb += u8"\\\"";
			else if (ch < 0x20 || ch == 0x7F)
			{
				sprintf((char *)buf, "\\x%02X", ch);
				m_sb += buf;
			}
			else
				m_sb += ch;
		}
	}

	return m_sb.ToString();
}

std::u8string GameStringUtil::Encode(std::u8string_view p_str)
{
	m_sb.Clear();
	m_pos = 0;
	m_str = p_str;

	while (m_pos < m_str.size())
	{
		char8_t ch = m_str[m_pos++];
		if (ch == '<')
		{
			m_sb += ParseTag();
		}
		else
		{
			if (ch == '\\')
			{
				ch = m_str[m_pos++];
				if (ch == 'n')
				{
					m_sb += '\n';
				}
				else if (ch == 'r')
				{
					m_sb += '\r';
				}
				else if (ch == 't')
				{
					m_sb += '\t';
				}
				else if (ch == 'x')
				{
					m_sb += xybase::string::stoi<char8_t>(std::u8string{ m_str.substr(m_pos, 2) });
					m_pos += 2;
				}
				else
					m_sb += ch;
			}
			else m_sb += ch;
		}
	}

	return m_sb.ToString();
}

std::u8string GameStringUtil::Parse(std::u8string_view p_str)
{
	return Encode(p_str);
}

long long GameStringUtil::DecodeMultibyteInteger(std::u8string_view p_str, int &p_outLength)
{
	char8_t type = p_str[0];
	if (type < 0xF0 || type == 0xFF)
		throw xybase::InvalidParameterException(L"m_str[m_pos]", L"Invalid integer specifier.", 145701);

	long long ret;

	switch (type)
	{
		/* [0] Int8 */
	case 0xF0:
		p_outLength = 2;
		return p_str[1];
		break;
		/* [0] 00 (Int16/Hi) */
	case 0xF1:
		p_outLength = 2;
		return ((long long)p_str[1]) << 8;
		break;
		/* [0] [1] (Int16BE) */
	case 0xF2:
		p_outLength = 3;
		ret = (long long)p_str[1] << 8;
		ret |= p_str[2];
		return ret;
		break;
		/* [0] 00 00 Int24-MSB */
	case 0xF3:
		p_outLength = 2;
		return (long long)p_str[1] << 16;
		break;
		/* [0] 00 [1] Int24-MSB&LSB */
	case 0xF4:
		p_outLength = 3;
		ret = (long long)p_str[1] << 16;
		ret |= p_str[2];
		return ret;
		break;
		/* [0] [1] 00 */
	case 0xF5:
		p_outLength = 3;
		ret = (long long)p_str[1] << 8;
		ret |= p_str[2];
		ret <<= 8;
		return ret;
		break;
		/* [0] [1] [2] Int24BE */
	case 0xF6:
		p_outLength = 4;
		ret = (long long)p_str[1] << 8;
		ret = (ret + p_str[2]) << 8;
		ret |= p_str[3];
		return ret;
		break;
		/* [0] 00 00 00 Int32-MSB*/
	case 0xF7:
		p_outLength = 2;
		return (long long)p_str[1] << 24;
		break;
		/* [0] 00 00 [1] Int32-MSB&LSB */
	case 0xF8:
		p_outLength = 3;
		ret = (long long)p_str[1] << 24;
		ret |= p_str[2];
		return ret;
		break;
		/* [0] 00 [1] 00 Int32/Hi-MSB & Int32/Lo-MSB */
	case 0xF9:
		p_outLength = 3;
		ret = (long long)p_str[1] << 16;
		ret |= p_str[2];
		ret <<= 8;
		return ret;
		break;
		/* [0] 00 [1] [2] */
	case 0xFA:
		p_outLength = 4;
		ret = (long long)p_str[1] << 16;
		ret = (ret + p_str[2]) << 8;
		ret |= p_str[3];
		return ret;
		break;
		/* [0] [1] 00 00 */
	case 0xFB:
		p_outLength = 3;
		ret = (long long)p_str[1] << 8;
		ret |= p_str[2];
		ret <<= 16;
		return ret;
		break;
		/* [0] [1] 00 [2] */
	case 0xFC:
		p_outLength = 4;
		ret = (long long)p_str[1] << 8;
		ret |= p_str[2];
		ret = (ret << 16) | p_str[3];
		return ret;
		break;
		/* [0] [1] [2] 00 */
	case 0xFD:
		p_outLength = 4;
		ret = (long long)p_str[1] << 8;
		ret = (ret + p_str[2]) << 8;
		ret = (ret + p_str[3]) << 8;
		return ret;
		break;
		/* [0] [1] [2] [3] Int32BE */
	case 0xFE:
		p_outLength = 5;
		ret = (long long)p_str[1] << 8;
		ret = (ret + p_str[2]) << 8;
		ret = (ret + p_str[3]) << 8;
		ret = (ret + p_str[4]);
		return ret;
		break;
	default:
		/* Something impossible happened */
		abort();
	}
}

long long GameStringUtil::DecodeInteger(std::u8string_view p_str, int &p_outLength)
{
	if (IsVariable(p_str[0])) throw xybase::InvalidParameterException(L"m_str", L"Unexpected variable in input string.", 16210);
	if (IsString(p_str[0])) throw xybase::InvalidParameterException(L"m_str", L"Unexpected string in input string.", 16211);

	if (IsMultiByteInteger(p_str[0]))
	{
		return DecodeMultibyteInteger(p_str, p_outLength);
	}
	p_outLength = 1;
	return p_str[0] - 1;
}

std::u8string GameStringUtil::DecodeString(std::u8string_view p_str, int &p_outLength)
{
	int step;
	int length = DecodeInteger(p_str, step);
	p_outLength = step + length;
	GameStringUtil gs;
	return u8'<' + gs.Decode(p_str.substr(step, length)) + u8'>';
}

long long GameStringUtil::ReadInteger()
{
	if (IsVariable(m_str[m_pos])) throw xybase::InvalidParameterException(L"m_str", L"Unexpected variable in input string.", 16208);
	if (IsString(m_str[m_pos])) throw xybase::InvalidParameterException(L"m_str", L"Unexpected string in input string.", 16209);
	if (IsMultiByteInteger(m_str[m_pos]))
	{
		int step;
		auto ret = DecodeMultibyteInteger(m_str.substr(m_pos, 5), step);
		m_pos += step;
		return ret;
	}
	return m_str[m_pos++] - 1;
}

bool GameStringUtil::IsLeadingFlag(const char8_t type)
{
	return (type >= 0xD8 && type <= 0xE5) || (type >= 0xE8 && type <= 0xEB) || (type >= 0xF0);
}

bool GameStringUtil::IsVariable(const char8_t type)
{
	return IsOperator(type) || IsParameterVariable(type) || IsTimeVariable(type);
}

std::u8string GameStringUtil::EncodeString()
{
	char8_t ch = m_str[m_pos++];
	char8_t end = m_str.find_first_of(',', m_pos);
	if (ch == '<')
	{
		int layer = 0;
		int p = m_pos;

		while ((1))
		{
			if (m_str[p] == '>')
			{
				if (layer == 0) break;
				--layer, ++p;
			}
			else if (m_str[p] == '<')
			{
				++layer, ++p;
			}
			else if (m_str[p] == '\\')
			{
				if (m_str[++p] == 'x') p += 2;
			}
			else ++p;
		}
		end = p;
	}

	m_pos = end + 1;
	return ch + std::u8string(m_str.substr(m_pos, end - m_pos));
}

std::u8string GameStringUtil::ParseTag()
{
	// 获取Tag
	size_t eot = m_str.find_first_of('(', m_pos);
	if (eot == std::u8string_view::npos) throw xybase::InvalidParameterException(L"m_str", L"Invalid tag start!", 126701);
	std::u8string_view tag = m_str.substr(m_pos, eot - m_pos);



	// 为读取参数做好准备
	m_pos = eot + 1;
}

void GameStringUtil::DecodeTag(const char8_t tag)
{
	TagDefinition *def = defs;

	int step;
	long long tagLength = DecodeInteger(m_str.substr(m_pos, 5), step);
	m_pos += step;

	std::u8string_view param(m_str.substr(m_pos, tagLength)); /* omit terminator */

	if (m_str.size() <= m_pos + tagLength)
	{
		std::wcerr << L"Weird position in " << xybase::string::to_wstring(std::u8string{ m_str }) << std::endl;
	}
	else if (m_str[m_pos + tagLength] != CONTROL_SEQ_END)
	{
		throw xybase::InvalidParameterException(L"m_str", L"No terminator found.", 119251);
	}

	m_pos += tagLength + 1; // prepare for next read

	while (def->name)
	{
		if (def->tag == tag)
		{
			m_sb += '<';
			m_sb += def->name;
			if (def->argCount)
			{
				m_sb += '(';
				DecodeParameter(def->argCount, def->argMax, param);
				m_sb += ')';
			}
			else
			{
				// 无法处理参数，但获取到了参数
				if (!param.empty())
					throw xybase::InvalidParameterException(L"m_str", L"No handler to handle the param.", 119252);
			}
			m_sb += '>';
			return;
		}

		++def;
	}

	m_sb += u8"<Unknown_";
	m_sb += xybase::string::itos<char8_t>(tag, 16);
	m_sb += '(';
	DecodeParameter(0, 255, param);
	m_sb += u8")>";

	std::wcerr << std::format(L"Unknown tag {:02X}.", (int)tag) << std::endl;

	// 放弃解析，原样输出，祈祷不炸
	// 算了炸一下直到我全部修好
	//throw xybase::InvalidParameterException(L"tag", std::format(L"Unknown tag {:02X}.", (int)tag), 176010);
	// m_sb += tag;
}

void GameStringUtil::DecodeParameter(int p_argCount, int p_argMax, std::u8string_view p_param)
{
	int decodedParameter = 0;

	int p = 0;

	while (p < p_param.size())
	{
		if (decodedParameter) m_sb += ',';

		int step;
		DecodeValue(p_param.substr(p), step);
		p += step;

		decodedParameter++;
	}
}

void GameStringUtil::DecodeValue(std::u8string_view p_val, int &p_outLength)
{
	// 确认类型
	if (IsLeadingFlag(p_val[0]))
	{
		int step = 0;

		if (IsOperator(p_val[0]))
		{
			switch (p_val[0])
			{
			case GreaterThan:
				m_sb += u8"@gt";
				break;
			case GreaterThanOrEqualTo:
				m_sb += u8"@ge";
				break;
			case LessThan:
				m_sb += u8"@lt";
				break;
			case LessThanOrEqualTo:
				m_sb += u8"@le";
				break;
			case Equal:
				m_sb += u8"@eq";
				break;
			case NotEqual:
				m_sb += u8"@ne";
				break;
			default:
				abort();
			}
			m_sb += '(';
			DecodeValue(p_val.substr(1), step);
			p_outLength = step + 1;
			m_sb += ',';
			DecodeValue(p_val.substr(p_outLength), step);
			p_outLength += step;
			m_sb += ')';
		}
		else if (IsTimeVariable(p_val[0]))
		{
			m_sb += u8"$time(";
			switch ((TimeVariable)p_val[0])
			{
			case GameStringUtil::TimeMilliSecond:
				m_sb += u8"msec";
				break;
			case GameStringUtil::TimeSecond:
				m_sb += u8"sec";
				break;
			case GameStringUtil::TimeMinute:
				m_sb += u8"min";
				break;
			case GameStringUtil::TimeHour:
				m_sb += u8"hour";
				break;
			case GameStringUtil::TimeMDay:
				m_sb += u8"mday";
				break;
			case GameStringUtil::TimeWDat:
				m_sb += u8"wday";
				break;
			case GameStringUtil::TimeMon:
				m_sb += u8"mon";
				break;
			case GameStringUtil::TimeYear:
				m_sb += u8"year";
				break;
			default:
				break;
			}
			m_sb += ')';
			p_outLength = 1;
		}
		else if (IsMultiByteInteger(p_val[0]))
		{
			auto res = DecodeMultibyteInteger(p_val.substr(0, 5), step);
			m_sb += '#';
			m_sb += xybase::string::itos<char8_t>(res);
			p_outLength = step;
		}
		else if (IsParameterVariable(p_val[0]))
		{
			if (IsStringVariable(p_val[0]))
			{
				if (p_val[0] == StringParameter)
				{
					m_sb += u8"$str_param(";
				}
				else
				{
					m_sb += u8"$obj_param(";
				}
				// m_sb += DecodeString(p_param.substr(p), step);
				DecodeValue(p_val.substr(1), step);
				m_sb += u8')';
				p_outLength = 1 + step;
			}
			else
			{
				switch ((ParameterVariable)p_val[0])
				{
				case IntegerParameter:
					m_sb += u8"$int_param(";
					break;
				case PlayerParameter:
					m_sb += u8"$plyr_param(";
					break;
				}
				DecodeValue(p_val.substr(1), step);
				m_sb += u8')';
				p_outLength = 1 + step;
			}
		}
		else if (IsString(p_val[0]))
		{
			m_sb += DecodeString(p_val.substr(1), step);
			p_outLength = 1 + step;
		}
		else abort();
	}
	else
	{
		m_sb += '#';
		m_sb += xybase::string::itos<char8_t>(p_val[0] - 1);
		p_outLength = 1;
	}
}

bool GameStringUtil::IsTimeVariable(const char8_t type)
{
	return type >= 0xD8 && type <= 0xDF;
}

bool GameStringUtil::IsOperator(const char8_t type)
{
	return type >= 0xE0 && type <= 0xE7;
}

bool GameStringUtil::IsStringVariable(const char8_t type)
{
	return type == StringParameter || type == ObjectParameter || type == 0xFF;
}

bool GameStringUtil::IsParameterVariable(const char8_t type)
{
	return type >= 0xE8 && type < 0xEC;
}

bool GameStringUtil::IsMultiByteInteger(const char8_t type)
{
	return type >= 0xF0 && type < 0xFF;
}

bool GameStringUtil::IsString(const char8_t type)
{
	return type == 0xFF;
}
