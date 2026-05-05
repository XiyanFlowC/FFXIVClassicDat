#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include "xybase/StringBuilder.h"

/**
 * @brief Decodes and encodes game strings containing control sequences.
 */
class GameStringUtil
{
protected:
	const static uint8_t CONTROL_SEQ_START = '\x02';
	const static uint8_t CONTROL_SEQ_END = '\x03';

	// The tag types (enum Tag) obtained from SaintCoinach.
	// From https://github.com/xivapi/SaintCoinach/blob/35b1060e65ca0e18ad915a1c308f0e5f37a2bbd3/SaintCoinach/Text/TagType.cs
	// Also ffxiv-classic-text-dump
	// https://github.com/transparentmask/ffxiv-classic-text-dump/blob/master/tag_type.py
	
	/**
	 * @brief 文本中嵌入的控制序列标签类型。
	 */
	enum Tag : uint8_t
	{
		NONE = 0x00,

		RESET_TIME = 0x06,
		TIME = 0x07,     /** TODO: It seems to set the time used further on. */
		IF = 0x08,
		SWITCH = 0x09,
		IF_EQUALS = 0x0C,
		UNKNOWN_0A = 0x0A,     /** TODO */
		LINE_FEED = 0x10,
		WAIT = 0x11, /** Not present anywhere in game data up to 2015.04.17.0001.0000 */

		ICON = 0x12,
		COLOR = 0x13,
		COLOR2 = 0x14,     /** TODO */
		SOFT_HYPHEN = 0x16,
		UNKNOWN_17 = 0x17,     /** TODO: Used exclusively in Japanese and at start of new lines. */
		EMPHASIS2 = 0x19,     /** TODO: See if this is bold, only used very little. 0x1A emphasis is italic. */
		EMPHASIS = 0x1A,
		INDENT = 0x1D,
		COMMAND_ICON = 0x1E,
		DASH = 0x1F,
		VALUE = 0x20,
		FORMAT = 0x22,
		TWO_DIGIT_VALUE = 0x24,     /** A single-digit value is formatted with a leading zero. */
		TIME2 = 0x25, /** Not present anywhere in game data up to 2015.04.17.0001.0000 */
		VALUE2 = 0x26,
		SHEET = 0x28,
		HIGHLIGHT = 0x29,
		CLICKABLE = 0x2B,     /** Seemingly anything that has an action associated with it (NPCs, PCs, Items, etc.) */
		SPLIT = 0x2C,
		UNKNOWN_2D = 0x2D,     /** TODO */
		FIXED = 0x2E,
		UNKNOWN_2F = 0x2F,     /** TODO */
		SHEET_JA = 0x30,
		SHEET_EN = 0x31,
		SHEET_DE = 0x32,
		SHEET_FR = 0x33,
		SHEET_CHS = 0x34,
		SHEET_CHT = 0x35,
		INSTANCE_CONTENT = 0x40,     /** Presumably so it can be clicked? */
		UI_FOREGROUND = 0x48,
		UI_GLOW = 0x49,
		RUBY_CHARACTERS = 0x4A,     /** Mostly used on Japanese, which means ... */
		ZERO_PADDED_VALUE = 0x50,
		UNKNOWN_60 = 0x60,     /** TODO: Used as prefix in Gold Saucer announcements. */
	};

	struct TagDefinition
	{
		const char8_t *name;
		int argCount;
		int argMax;
		Tag tag;
	};

	static TagDefinition s_defs[];

	/** From https://github.com/xivapi/SaintCoinach/blob/35b1060e65ca0e18ad915a1c308f0e5f37a2bbd3/SaintCoinach/Text/DecodeExpressionType.cs
	 *  I don't know if the ARR changed the way how to handle the integers,
	 *  but those definitions from SaintCoinach are WRONG for 1.23b
	 *  Adapted according to the analysis
	 */
	/**
	 * @brief Represents a time variable.
	 */
	enum TimeVariable : uint8_t
	{
		TIME_MILLI_SECOND = 0xD8,
		TIME_SECOND = 0xD9,
		TIME_MINUTE = 0xDA,
		TIME_HOUR = 0xDB,
		TIME_M_DAY = 0xDC,
		TIME_W_DAY = 0xDD,
		TIME_MON = 0xDE,
		TIME_YEAR = 0xDF,
	};

	static bool IsTimeVariable(uint8_t type);

	/**
	 * @brief Represents an operator.
	 */
	enum Operator : uint8_t
	{
		GREATER_THAN_OR_EQUAL_TO = 0xE0,    /** Followed by two variables */
		GREATER_THAN = 0xE1,                 /** Followed by one variable */
		LESS_THAN_OR_EQUAL_TO = 0xE2,        /** Followed by two variables */
		LESS_THAN = 0xE3,                    /** Followed by one variable */
		EQUAL = 0xE4,                        /** Followed by two variables */
		NOT_EQUAL = 0xE5,                    /** Followed by two variables */
	};

	static bool IsOperator(uint8_t type);

	/**
	 * @brief Represents a parameter.
	 */
	enum ParameterVariable : uint8_t
	{
		/** TODO: I /think/ I got these right. */
		INTEGER_PARAMETER = 0xE8,        /** Followed by one variable */
		PLAYER_PARAMETER = 0xE9,         /** Followed by one variable */
		STRING_PARAMETER = 0xEA,         /** Followed by one variable */
		OBJECT_PARAMETER = 0xEB,         /** Followed by one variable */
		/** ReservedParameter = 0xEC, */
	};

	static bool IsStringVariable(uint8_t type);

	static bool IsParameterVariable(uint8_t type);

	static bool IsMultiByteInteger(uint8_t type);

	static bool IsString(uint8_t type);

	static long long DecodeMultibyteInteger(std::string_view p_str, int &p_outLength);

	static std::string EncodeMultibyteInteger(long long p_in);

	static std::u8string DecodeString(std::string_view p_str, int &p_outLength);

	static bool IsLeadingFlag(uint8_t type);

	static bool IsVariable(uint8_t type);

public:
	std::u8string Decode(std::string_view p_str);

	std::string Encode(const char8_t *p_str);
	std::string Parse(const char8_t *p_str);


	static long long DecodeInteger(std::string_view p_str, int &p_outLength);

	static std::string EncodeInteger(long long p_in);
private:

	long long ReadInteger();

	void DecodeTag(const uint8_t tag);

	void DecodeParameter(int p_argCount, int p_argMax, std::string_view p_param);

	void DecodeValue(std::string_view p_val, int &p_outLength);

	std::string ParseString();

	std::string ParseNumber();

	std::string ParseTag();

	std::string ParseParameter();

	std::string ParseValue();

	std::string ParseExpression();

	std::string ParseVariable();


protected:
	xybase::StringBuilder<char> m_sb;
	int m_pos = 0;
	std::string_view m_str;
};
