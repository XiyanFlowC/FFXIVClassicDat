#pragma once

#include <string>
#include <functional>

class GameStringUtil
{
	const static char8_t CONTROL_SEQ_START = u8'\x02';
	const static char8_t CONTROL_SEQ_END = u8'\x03';

    // The tag types (enum Tag) grabed from SaintCoinach
    // From https://github.com/xivapi/SaintCoinach/blob/35b1060e65ca0e18ad915a1c308f0e5f37a2bbd3/SaintCoinach/Text/TagType.cs
    // Also ffxiv-classic-text-dump
    // https://github.com/transparentmask/ffxiv-classic-text-dump/blob/master/tag_type.py
	enum Tag : char8_t
	{
        None = 0x00,

        /* Appears to set values for an input time.
         * - 222 / DEh  Year
         * - 221 / DDh  Month
         * - 220 / DCh  Day of week
         * - 219 / DBh  Day of month
         * - 218 / DAh  Hour
         * - 217 / D9h  Minute
         */
        ResetTime = 0x06,
        Time = 0x07,     // TODO: It seems to set the time used further on.
        If = 0x08,
        Switch = 0x09,
        IfEquals = 0x0C,
        Unknown0A = 0x0A,     // TODO
        LineBreak = 0x10,
        Wait = 0x11, // Not present anywhere in game data up to 2015.04.17.0001.0000

        Gui = 0x12,
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
        //Time = 0x25, // Not present anywhere in game data up to 2015.04.17.0001.0000
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
        std::function<void(GameStringUtil &, int, int, void *)> parser;
        int paramNum;
        int paramMax;
        Tag tag;
    };

    static TagDefinition defs;

public:
	std::u8string Decode(std::u8string_view p_str);

	std::u8string ProcessTag(const char8_t tag);

	std::u8string Encode(std::u8string_view p_str);

	std::u8string ParseTag(std::u8string_view p_tag);

protected:
    int m_pos;
    std::u8string_view m_str;
};

