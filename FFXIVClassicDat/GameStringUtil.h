#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include "xybase/StringBuilder.h"

/**
 * @brief 解码/编码游戏含游戏控制符的类。
 */
class GameStringUtil
{
protected:
	const static uint8_t CONTROL_SEQ_START = '\x02';
	const static uint8_t CONTROL_SEQ_END = '\x03';

	// The tag types (enum Tag) grabed from SaintCoinach
	// From https://github.com/xivapi/SaintCoinach/blob/35b1060e65ca0e18ad915a1c308f0e5f37a2bbd3/SaintCoinach/Text/TagType.cs
	// Also ffxiv-classic-text-dump
	// https://github.com/transparentmask/ffxiv-classic-text-dump/blob/master/tag_type.py
	enum Tag : uint8_t
	{
		None = 0x00,

		ResetTime = 0x06,
		Time = 0x07,     // TODO: It seems to set the time used further on.
		If = 0x08,
		Switch = 0x09,
		IfEquals = 0x0C,
		Unknown0A = 0x0A,     // TODO
		LineFeed = 0x10,
		Wait = 0x11, // Not present anywhere in game data up to 2015.04.17.0001.0000

		Icon = 0x12,
		Color = 0x13,
		Color2 = 0x14,     // TODO
		SoftHyphen = 0x16,
		Unknown17 = 0x17,     // TODO: Used exclusively in Japanese and at start of new lines.
		Emphasis2 = 0x19,     // TODO: See if this is bold, only used very little. 0x1A emphasis is italic.
		Emphasis = 0x1A,
		Indent = 0x1D,
		CommandIcon = 0x1E,
		Dash = 0x1F,
		Value = 0x20,
		Format = 0x22,
		TwoDigitValue = 0x24,     // A single-digit value is formatted with a leading zero. 
		Time2 = 0x25, // Not present anywhere in game data up to 2015.04.17.0001.0000
		Value2 = 0x26,
		Sheet = 0x28,
		Highlight = 0x29,
		Clickable = 0x2B,     // Seemingly anything that has an action associated with it (NPCs, PCs, Items, etc.)
		Split = 0x2C,
		Unknown2D = 0x2D,     // TODO
		Fixed = 0x2E,
		Unknown2F = 0x2F,     // TODO
		SheetJa = 0x30,
		SheetEn = 0x31,
		SheetDe = 0x32,
		SheetFr = 0x33,
		SheetChs = 0x34,
		SheetCht = 0x35,
		InstanceContent = 0x40,     // Presumably so it can be clicked?
		UIForeground = 0x48,
		UIGlow = 0x49,
		RubyCharaters = 0x4A,     // Mostly used on Japanese, which means 
		ZeroPaddedValue = 0x50,
		Unknown60 = 0x60,     // TODO: Used as prefix in Gold Saucer announcements.
	};

	struct TagDefinition
	{
		const char8_t *name;
		int argCount;
		int argMax;
		Tag tag;
	};

	static TagDefinition defs[];

	// From https://github.com/xivapi/SaintCoinach/blob/35b1060e65ca0e18ad915a1c308f0e5f37a2bbd3/SaintCoinach/Text/DecodeExpressionType.cs
	// I don't know if the ARR changed the way how to handle the integers,
	// but those definitions from SaintCoinach are WRONG for 1.23b
	// Adapted according to the analysis
	/**
		* @brief 表示时间变量
		*/
	enum TimeVariable : uint8_t
	{
		TimeMilliSecond = 0xD8,
		TimeSecond = 0xD9,
		TimeMinute = 0xDA,
		TimeHour = 0xDB,
		TimeMDay = 0xDC,
		TimeWDay = 0xDD,
		TimeMon = 0xDE,
		TimeYear = 0xDF,
	};

	static bool IsTimeVariable(uint8_t type);

	/**
	* @brief 表示算子
	*/
	enum Operator : uint8_t
	{
		GreaterThanOrEqualTo = 0xE0,    // Followed by two variables
		GreaterThan = 0xE1,             // Followed by one variable
		LessThanOrEqualTo = 0xE2,       // Followed by two variables
		LessThan = 0xE3,                // Followed by one variable
		Equal = 0xE4,                   // Followed by two variables
		NotEqual = 0xE5,                // Followed by two variables
	};

	static bool IsOperator(uint8_t type);

	/**
		* @brief 表示参数
		*/
	enum ParameterVariable : uint8_t
	{
		// TODO: I /think/ I got these right.
		IntegerParameter = 0xE8,        // Followed by one variable
		PlayerParameter = 0xE9,         // Followed by one variable
		StringParameter = 0xEA,         // Followed by one variable
		ObjectParameter = 0xEB,         // Followed by one variable
		// ReservedParameter = 0xEC,
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

