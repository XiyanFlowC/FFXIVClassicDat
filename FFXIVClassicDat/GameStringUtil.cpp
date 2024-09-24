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
	{u8"Value2", 1, 1, GameStringUtil::Tag::Value2},
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
	{u8"Time2", 1, 255, Tag::Time2},
	{u8"Indent", 1, 255, Tag::Indent},
	{u8"Dash", 1, 255, Tag::Dash},
	{u8"TwoDigitValue", 1, 255, Tag::TwoDigitValue},
	{nullptr, 0, 0, GameStringUtil::Tag::None},
};

std::u8string GameStringUtil::Decode(std::string_view p_str)
{
	m_str = p_str;
	m_pos = 0;
	m_sb.Clear();
	const char buf[8] = { 0 };
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
			char ch = p_str[m_pos++];
			if (ch == '\\')
				m_sb += "\\\\";
			else if (ch == '<')
				m_sb += "\\<";
			else if (ch == '>')
				m_sb += "\\>";
			/*else if (ch == ',')
				m_sb += u8"\\,";*/
			else if (ch == '"')
				m_sb += "\\\"";
			else if ((ch > 0 && ch < 0x20) || ch == 0x7F)
			{
				sprintf((char *)buf, "\\x%02X", ch & 0xFF);
				m_sb += buf;
			}
			else
				m_sb += ch;
		}
	}

	return (char8_t *)m_sb.ToString();
}

std::string GameStringUtil::Encode(const char8_t *p_str)
{
	m_sb.Clear();
	m_pos = 0;
	m_str = (const char *)p_str;

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
					m_sb += xybase::string::stoi(std::string{ m_str.substr(m_pos, 2) });
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

std::string GameStringUtil::Parse(const char8_t *p_str)
{
	return Encode(p_str);
}

long long GameStringUtil::DecodeMultibyteInteger(std::string_view p_str, int &p_outLength)
{
	// holy fuck the char is signed?!!
	uint8_t type = p_str[0];
	if (type < 0xF0 || type == 0xFF)
		throw xybase::InvalidParameterException(L"m_str[m_pos]", L"Invalid integer specifier.", 145701);

	long long ret;

	switch ((uint8_t)type)
	{
		/* [0] Int8 */
	case 0xF0:
		p_outLength = 2;
		return (uint8_t)p_str[1];
		break;
		/* [0] 00 (Int16/Hi) */
	case 0xF1:
		p_outLength = 2;
		return ((long long)(uint8_t)p_str[1]) << 8;
		break;
		/* [0] [1] (Int16BE) */
	case 0xF2:
		p_outLength = 3;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret |= (uint8_t)p_str[2];
		return ret;
		break;
		/* [0] 00 00 Int24-MSB */
	case 0xF3:
		p_outLength = 2;
		return (long long)(uint8_t)p_str[1] << 16;
		break;
		/* [0] 00 [1] Int24-MSB&LSB */
	case 0xF4:
		p_outLength = 3;
		ret = (long long)(uint8_t)p_str[1] << 16;
		ret |= (uint8_t)p_str[2];
		return ret;
		break;
		/* [0] [1] 00 */
	case 0xF5:
		p_outLength = 3;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret |= (uint8_t)p_str[2];
		ret <<= 8;
		return ret;
		break;
		/* [0] [1] [2] Int24BE */
	case 0xF6:
		p_outLength = 4;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret = (ret + (uint8_t)p_str[2]) << 8;
		ret |= (uint8_t)p_str[3];
		return ret;
		break;
		/* [0] 00 00 00 Int32-MSB*/
	case 0xF7:
		p_outLength = 2;
		return (long long)(uint8_t)p_str[1] << 24;
		break;
		/* [0] 00 00 [1] Int32-MSB&LSB */
	case 0xF8:
		p_outLength = 3;
		ret = (long long)(uint8_t)p_str[1] << 24;
		ret |= (uint8_t)p_str[2];
		return ret;
		break;
		/* [0] 00 [1] 00 Int32/Hi-MSB & Int32/Lo-MSB */
	case 0xF9:
		p_outLength = 3;
		ret = (long long)(uint8_t)p_str[1] << 16;
		ret |= (uint8_t)p_str[2];
		ret <<= 8;
		return ret;
		break;
		/* [0] 00 [1] [2] */
	case 0xFA:
		p_outLength = 4;
		ret = (long long)(uint8_t)p_str[1] << 16;
		ret = (ret + (uint8_t)p_str[2]) << 8;
		ret |= (uint8_t)p_str[3];
		return ret;
		break;
		/* [0] [1] 00 00 */
	case 0xFB:
		p_outLength = 3;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret |= (uint8_t)p_str[2];
		ret <<= 16;
		return ret;
		break;
		/* [0] [1] 00 [2] */
	case 0xFC:
		p_outLength = 4;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret |= (uint8_t)p_str[2];
		ret = (ret << 16) | (uint8_t)p_str[3];
		return ret;
		break;
		/* [0] [1] [2] 00 */
	case 0xFD:
		p_outLength = 4;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret = (ret + (uint8_t)p_str[2]) << 8;
		ret = (ret + (uint8_t)p_str[3]) << 8;
		return ret;
		break;
		/* [0] [1] [2] [3] Int32BE */
	case 0xFE:
		p_outLength = 5;
		ret = (long long)(uint8_t)p_str[1] << 8;
		ret = (ret + (uint8_t)p_str[2]) << 8;
		ret = (ret + (uint8_t)p_str[3]) << 8;
		ret = (ret + (uint8_t)p_str[4]);
		return ret;
		break;
	default:
		/* Something impossible happened */
		abort();
	}
}

long long GameStringUtil::DecodeInteger(std::string_view p_str, int &p_outLength)
{
	if (IsVariable(p_str[0])) throw xybase::InvalidParameterException(L"m_str", L"Unexpected variable in input string.", 16210);
	if (IsString(p_str[0])) throw xybase::InvalidParameterException(L"m_str", L"Unexpected string in input string.", 16211);

	if (IsMultiByteInteger(p_str[0]))
	{
		return DecodeMultibyteInteger(p_str, p_outLength);
	}
	p_outLength = 1;
	return (uint8_t)p_str[0] - 1;
}

std::string GameStringUtil::EncodeInteger(long long p_in)
{
	if (p_in >= 0 && p_in < 0xFF)
	{
		if (IsLeadingFlag(p_in + 1)) return EncodeMultibyteInteger(p_in);
		std::string ret("");
		return ret + (char)(p_in + 1);
	}
	return EncodeMultibyteInteger(p_in);
}

std::string GameStringUtil::EncodeMultibyteInteger(long long p_in)
{
	if (p_in > 0xFFFFFFFFll || p_in < -0x100000000ll || p_in == 0)
		throw xybase::InvalidParameterException(L"p_in", std::format(L"Value {} cannot be encoded.", p_in), 16255);
	
	uint32_t val = (uint32_t)p_in;
	xybase::StringBuilder sb(5);
	if (!(val & ~0xFF))
	{
		sb += (char)0xF0;
		sb += (char)val;
	}
	else if (!(val & ~0xFF00))
	{
		sb += (char)0xF1;
		sb += (char)(val >> 8);
	}
	else if (!(val & ~0xFFFF))
	{
		sb += (char)0xF2;
		sb += (char)(val >> 8);
		sb += (char)val;
	}
	else if (!(val & ~0xFF0000))
	{
		sb += (char)0xF3;
		sb += (char)(val >> 16);
	}
	else if (!(val & ~0xFF00FF))
	{
		sb += (char)0xF4;
		sb += (char)(val >> 16);
		sb += (char)val;
	}
	else if (!(val & ~0xFFFF00))
	{
		sb += (char)0xF5;
		sb += (char)(val >> 16);
		sb += (char)(val >> 8);
	}
	else if (!(val & ~0xFFFFFF))
	{
		sb += (char)0xF6;
		sb += (char)(val >> 16);
		sb += (char)(val >> 8);
		sb += (char)val;
	}
	else if (!(val & ~0xFF000000))
	{
		sb += (char)0xF7;
		sb += (char)(val >> 24);
	}
	else if (!(val & ~0xFF0000FF))
	{
		sb += (char)0xF8;
		sb += (char)(val >> 24);
		sb += (char)val;
	}
	else if (!(val & ~0xFF00FF00))
	{
		sb += (char)0xF9;
		sb += (char)(val >> 24);
		sb += (char)(val >> 8);
	}
	else if (!(val & ~0xFF00FFFF))
	{
		sb += (char)0xFA;
		sb += (char)(val >> 24);
		sb += (char)(val >> 8);
		sb += (char)val;
	}
	else if (!(val & ~0xFFFF0000))
	{
		sb += (char)0xFB;
		sb += (char)(val >> 24);
		sb += (char)(val >> 16);
	}
	else if (!(val & ~0xFFFF00FF))
	{
		sb += (char)0xFC;
		sb += (char)(val >> 24);
		sb += (char)(val >> 16);
		sb += (char)val;
	}
	else if (!(val & ~0xFFFFFF00))
	{
		sb += (char)0xFD;
		sb += (char)(val >> 24);
		sb += (char)(val >> 16);
		sb += (char)(val >> 8);
	}
	else if (!(val & ~0xFFFFFFFF))
	{
		sb += (char)0xFE;
		sb += (char)(val >> 24);
		sb += (char)(val >> 16);
		sb += (char)(val >> 8);
		sb += (char)val;
	}
	else abort();
	return sb.ToString();
}

std::u8string GameStringUtil::DecodeString(std::string_view p_str, int &p_outLength)
{
	int step;
	int length = DecodeInteger(p_str, step);
	p_outLength = step + length;
	GameStringUtil gs;
	auto res = gs.Decode(p_str.substr(step, length));
	if (res.find_first_of(u8",)") != std::u8string::npos)
		return u8'<' + res + u8'>';
	else
		return res;
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

bool GameStringUtil::IsLeadingFlag(uint8_t type)
{
	return (type >= 0xD8 && type <= 0xE5) || (type >= 0xE8 && type <= 0xEB) || (type >= 0xF0);
}

bool GameStringUtil::IsVariable(uint8_t type)
{
	return IsOperator(type) || IsParameterVariable(type) || IsTimeVariable(type);
}

std::string GameStringUtil::ParseString()
{
	char8_t ch = m_str[m_pos];
	auto end = m_str.find_first_of(",)", m_pos);
	if (end == std::string_view::npos)
		throw xybase::InvalidParameterException(L"m_str", L"Not terminator found.", 141401);
	if (ch == '<')
	{
		int layer = 0;
		int p = ++m_pos;

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

	GameStringUtil gsu;
	auto encodedStr = gsu.Encode((char8_t *)std::string(m_str.substr(m_pos, end - m_pos)).c_str());
	std::string ret("\xFF");
	ret += EncodeInteger(encodedStr.size());
	ret += encodedStr;
	m_pos = end;
	if (m_str[m_pos] == '>') ++m_pos;
	return ret;
}

std::string GameStringUtil::ParseNumber()
{
	assert(m_str[m_pos] == '#');
	m_pos++;
	auto end = m_str.find_first_of(",)", m_pos);
	std::string ret;
	if (m_str[m_pos] == 'x')
	{
		ret = EncodeInteger(xybase::string::stoi(std::string{ m_str.substr(m_pos + 1, end - m_pos - 1) }, 16));
	}
	else
	{
		ret = EncodeInteger(xybase::string::stoi(std::string{ m_str.substr(m_pos, end - m_pos) }));
	}
	m_pos = end;
	return ret;
}

std::string GameStringUtil::ParseTag()
{
	// 获取Tag
	size_t eot = m_str.find_first_of("(>", m_pos);
	if (eot == std::string_view::npos) throw xybase::InvalidParameterException(L"m_str", L"Invalid tag start!", 126701);
	std::string_view tag = m_str.substr(m_pos, eot - m_pos);
	// 为读取参数/继续读取做好准备
	m_pos = eot + 1;

	auto *ptr = defs;
	while (ptr->name)
	{
		if ((const char *)ptr->name == tag)
		{
			std::string ret("\x02");
			ret += (char8_t)ptr->tag;
			if (ptr->argCount)
			{
				if (m_str[eot] == '>') throw xybase::InvalidParameterException(L"m_str[eot]", L"Argument count is zero but find parameter!", 126702);

				std::string param = ParseParameter();

				if (m_str[m_pos] != '>') 
					throw xybase::InvalidParameterException(L"m_str[eot]",
						std::format(L"Tag terminator '>' expected, but found {}. (Near {})", (char)m_str[m_pos], 
							xybase::string::to_wstring(std::string(m_str.substr(std::max(m_pos - 16, 0), 32)))), 126703);
				m_pos++;

				ret += EncodeInteger(param.size());
				ret += param;
			}
			else 
				ret += EncodeInteger(0);
			ret += '\x03';
			return ret;
		}

		++ptr;
	}
	throw xybase::InvalidParameterException(L"m_str", L"Unknown tag.", 162558);
}

std::string GameStringUtil::ParseParameter()
{
	if (m_str[m_pos] == ')')
	{
		m_pos++;
		return "";
	}
	xybase::StringBuilder sb;
	while (m_pos < m_str.size())
	{
		sb += ParseValue();
		if (m_str[m_pos] != ',' && m_str[m_pos] != ')')
			throw xybase::InvalidParameterException(L"m_str[m_pos]",
				std::format(L"Expected ',' or ')', but got {}", (char)m_str[m_pos]), 51201);
		if (m_str[m_pos++] == ')') break;
	}
	return sb.ToString();
}

std::string GameStringUtil::ParseValue()
{
	if (m_str[m_pos] == '@')
		return ParseExpression();
	else if (m_str[m_pos] == '$')
		return ParseVariable();
	else if (m_str[m_pos] == '<')
		return ParseString();
	else if (m_str[m_pos] == '#')
		return ParseNumber();
	else 
		return ParseString();
}

std::string GameStringUtil::ParseExpression()
{
	assert(m_str[m_pos] == '@');
	m_pos++;

	std::string ret;

	auto op = m_str.substr(m_pos, 2);
	if (op == "lt")
	{
		ret += Operator::LessThan;
	}
	else if (op == "gt")
	{
		ret += Operator::GreaterThan;
	}
	else if (op == "ge")
	{
		ret += Operator::GreaterThanOrEqualTo;
	}
	else if (op == "le")
	{
		ret += Operator::LessThanOrEqualTo;
	}
	else if (op == "eq")
	{
		ret += Operator::Equal;
	}
	else if (op == "ne")
	{
		ret += Operator::NotEqual;
	}
	else
		throw xybase::InvalidParameterException(L"op", L"Invalid operator.", 56501);
	m_pos += 2;
	if (m_str[m_pos++] != '(')
		throw xybase::InvalidParameterException(L"op", std::wstring(L"Expected '(', but got ") + (wchar_t)m_str[--m_pos], 56502);
	ret += ParseValue();
	if (m_str[m_pos++] != ',')
		throw xybase::InvalidParameterException(L"op", std::wstring(L"Expected ',', but got ") + (wchar_t)m_str[--m_pos], 56503);
	ret += ParseValue();
	if (m_str[m_pos++] != ')')
		throw xybase::InvalidParameterException(L"op", std::wstring(L"Expected ')', but got ") + (wchar_t)m_str[--m_pos], 56503);

	return ret;
}

std::string GameStringUtil::ParseVariable()
{
	assert(m_str[m_pos] == '$');
	m_pos++;
	auto end = m_str.find_first_of('(', m_pos);
	auto category = m_str.substr(m_pos, end - m_pos);
	std::string ret;
	m_pos = end + 1;
	if (category == "time")
	{
		auto pend = m_str.find_first_of(')', m_pos);
		auto type = m_str.substr(m_pos, pend - m_pos);
		m_pos = pend;
		if (type == "msec")
			ret += TimeMilliSecond;
		else if (type == "sec")
			ret += TimeSecond;
		else if (type == "min")
			ret += TimeMinute;
		else if (type == "hour")
			ret += TimeHour;
		else if (type == "wday")
			ret += TimeWDay;
		else if (type == "mday")
			ret += TimeMDay;
		else if (type == "mon")
			ret += TimeMon;
		else if (type == "year")
			ret += TimeYear;
		else
			throw xybase::InvalidParameterException(L"TimeType",
				std::format(L"Unknown Time Type {}", xybase::string::to_wstring(std::string(type))), 60801);
	}
	else {
		if (category == "plyr")
		{
			ret += PlayerParameter;
		}
		else if (category == "int")
		{
			ret += IntegerParameter;
		}
		else if (category == "obj")
		{
			ret += ObjectParameter;
		}
		else if (category == "str")
		{
			ret += StringParameter;
		}
		else
			throw xybase::InvalidParameterException(L"category",
				std::format(L"Unknown category {}", xybase::string::to_wstring(std::string(category))), 60801);

		ret += ParseValue();
	}
	
	if (m_str[m_pos++] != ')')
		throw xybase::InvalidParameterException(L"category",
			std::format(L"Expected ')', but got {}", m_str[m_pos - 1]), 60802);
	return ret;
}

void GameStringUtil::DecodeTag(const uint8_t tag)
{
	TagDefinition *def = defs;

	int step;
	long long tagLength = DecodeInteger(m_str.substr(m_pos, 5), step);
	m_pos += step;

	std::string_view param(m_str.substr(m_pos, tagLength)); /* omit terminator */

	if (m_str.size() <= m_pos + tagLength)
	{
		std::wcerr << L"Weird position in " << xybase::string::to_wstring(std::string{ m_str }) << std::endl;
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
			m_sb += (char *)def->name;
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

	m_sb += "<Unknown_";
	m_sb += xybase::string::itos(tag, 16);
	m_sb += '(';
	DecodeParameter(0, 255, param);
	m_sb += ")>";

	std::wcerr << std::format(L"Unknown tag {:02X}.", (int)tag) << std::endl;

	// 放弃解析，原样输出，祈祷不炸
	// 算了炸一下直到我全部修好
	//throw xybase::InvalidParameterException(L"tag", std::format(L"Unknown tag {:02X}.", (int)tag), 176010);
	// m_sb += tag;
}

void GameStringUtil::DecodeParameter(int p_argCount, int p_argMax, std::string_view p_param)
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

void GameStringUtil::DecodeValue(std::string_view p_val, int &p_outLength)
{
	// 确认类型
	if (IsLeadingFlag(p_val[0]))
	{
		int step = 0;

		if (IsOperator(p_val[0]))
		{
			switch ((Operator)p_val[0])
			{
			case GreaterThan:
				m_sb += "@gt";
				break;
			case GreaterThanOrEqualTo:
				m_sb += "@ge";
				break;
			case LessThan:
				m_sb += "@lt";
				break;
			case LessThanOrEqualTo:
				m_sb += "@le";
				break;
			case Equal:
				m_sb += "@eq";
				break;
			case NotEqual:
				m_sb += "@ne";
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
			m_sb += "$time(";
			switch ((TimeVariable)p_val[0])
			{
			case GameStringUtil::TimeMilliSecond:
				m_sb += "msec";
				break;
			case GameStringUtil::TimeSecond:
				m_sb += "sec";
				break;
			case GameStringUtil::TimeMinute:
				m_sb += "min";
				break;
			case GameStringUtil::TimeHour:
				m_sb += "hour";
				break;
			case GameStringUtil::TimeMDay:
				m_sb += "mday";
				break;
			case GameStringUtil::TimeWDay:
				m_sb += "wday";
				break;
			case GameStringUtil::TimeMon:
				m_sb += "mon";
				break;
			case GameStringUtil::TimeYear:
				m_sb += "year";
				break;
			default:
				abort();
			}
			m_sb += ')';
			p_outLength = 1;
		}
		else if (IsMultiByteInteger(p_val[0]))
		{
			auto res = DecodeMultibyteInteger(p_val.substr(0, 5), step);
			m_sb += '#';
			m_sb += xybase::string::itos(res);
			p_outLength = step;
		}
		else if (IsParameterVariable(p_val[0]))
		{
			switch ((ParameterVariable)p_val[0])
			{
			case StringParameter:
				m_sb += "$str(";
				break;
			case ObjectParameter:
				m_sb += "$obj(";
				break;
			case IntegerParameter:
				m_sb += "$int(";
				break;
			case PlayerParameter:
				m_sb += "$plyr(";
				break;
			default:
				abort();
			}
			DecodeValue(p_val.substr(1), step);
			m_sb += ')';
			p_outLength = 1 + step;
		}
		else if (IsString(p_val[0]))
		{
			m_sb += (char *)DecodeString(p_val.substr(1), step).c_str();
			p_outLength = 1 + step;
		}
		else abort();
	}
	else
	{
		m_sb += '#';
		m_sb += xybase::string::itos((uint8_t)p_val[0] - 1);
		p_outLength = 1;
	}
}

bool GameStringUtil::IsTimeVariable(uint8_t type)
{
	return type >= 0xD8 && type <= 0xDF;
}

bool GameStringUtil::IsOperator(uint8_t type)
{
	return type >= 0xE0 && type <= 0xE7;
}

bool GameStringUtil::IsStringVariable(uint8_t type)
{
	return type == StringParameter || type == ObjectParameter || type == 0xFF;
}

bool GameStringUtil::IsParameterVariable(uint8_t type)
{
	return type >= 0xE8 && type < 0xEC;
}

bool GameStringUtil::IsMultiByteInteger(uint8_t type)
{
	return type >= 0xF0 && type < 0xFF;
}

bool GameStringUtil::IsString(uint8_t type)
{
	return type == 0xFF;
}
