#include "pch.h"

#include <string>
#include "GameStringUtil.h"

TEST(GameStringTest, DecodePlain) {
	GameStringUtil gsu;
	std::string input = "Test input\x02\x10\x01\x03New \x02\x08\x17\xE4\xE8\x02\x04\xFF\x05line\xFF\x0B\x02\x29\x07\xFF\x05LINE\x03\x03.";
	std::string output = "Test input<LF>New <If(@eq($int(#1),#3),line,<<Highlight(LINE)>>)>.";
	std::string actualOutput = (char *)gsu.Decode(input).c_str();
	EXPECT_EQ(output, actualOutput);
}

TEST(GameStringTest, DecodeOnePara) {
	GameStringUtil gsu;
	std::string input = "\x02\x2C\x0C\xFF\x06\x02\x29\x02\x02\x03\xFF\x02 \x02\x03";
	std::string output = "<Split(<<Highlight(#1)>>, ,#1)>";
	std::string actualOutput = (char *)gsu.Decode(input).c_str();
	EXPECT_EQ(output, actualOutput);
}

TEST(GameStringTest, EncodePlain)
{
	GameStringUtil gsu;
	std::u8string coded = u8"Test input<LF>New <If(@eq($int(#1),#3),<line>,<<Highlight(LINE)>>)>.";
	std::string raw = "Test input\x02\x10\x01\x03New \x02\x08\x17\xE4\xE8\x02\x04\xFF\x05line\xFF\x0B\x02\x29\x07\xFF\x05LINE\x03\x03.";
	std::string actualOutput = (char *)gsu.Encode(coded.c_str()).c_str();
	EXPECT_EQ(raw, actualOutput);
}

TEST(GameStringTest, Codec)
{
	GameStringUtil gsu;
	auto ori = u8"<If(@lt(#2,$int(#65535)),Test if,<Word <Highlight(<<Sheet(xtx/test,$int(#1),#996)>>)>>)>. Test <Highlight(Linefeed<LF>in<LF>tag.)>";
	std::string raw = gsu.Encode(ori);
	auto text = gsu.Decode(raw);
	EXPECT_STREQ((char *)text.c_str(), (char *)ori);
	EXPECT_EQ(gsu.Encode(text.c_str()), raw);
}

TEST(GameStringTest, IntegerEncode)
{
	EXPECT_EQ(std::string("\xFE\x12\x34\x56\x78"), GameStringUtil::EncodeInteger(0x12345678));
	EXPECT_EQ(std::string("\xF9\x12\x56"), GameStringUtil::EncodeInteger(0x12005600));
	EXPECT_EQ(std::string("\x79"), GameStringUtil::EncodeInteger(0x78));
	EXPECT_EQ(std::string("\xF0\xE7"), GameStringUtil::EncodeInteger(0xE7));
	EXPECT_EQ(std::string("\xEC"), GameStringUtil::EncodeInteger(0xEB));
	EXPECT_EQ(std::string("\xF7\xEC"), GameStringUtil::EncodeInteger(0xEC000000));
}

