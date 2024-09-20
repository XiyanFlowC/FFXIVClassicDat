#include "pch.h"

#include <string>
#include "GameStringUtil.h"

TEST(GameStringTest, DecodePlain) {
	GameStringUtil gsu;
	std::u8string input = (char8_t*)"Test input\x02\x10\x01\x03New \x02\x08\x17\xE4\xE8\x02\x04\xFF\x05line\xFF\x0B\x02\x29\x07\xFF\x05LINE\x03\x03.";
	std::string output = "Test input<LF>New <If(@eq($int_param(#1),#3),<line>,<<Highlight(<LINE>)>>)>.";
	std::string actualOutput = (char *)gsu.Decode(input).c_str();
	EXPECT_EQ(output, actualOutput);
}

TEST(GameStringTest, EncodePlain)
{
	GameStringUtil gsu;
	std::string raw = "Test input\x02\x10\x01\x03New \x02\x08\x17\xE4\xE8\x02\x04\xFF\x05line\xFF\x0B\x02\x29\x07\xFF\x05LINE\x03\x03.";
	std::u8string coded = u8"Test input<LF>New <If(@eq($int_param(#1),#3),<line>,<<Highlight(<LINE>)>>)>.";
	std::string actualOutput = (char *)gsu.Encode(coded).c_str();
	EXPECT_EQ(raw, actualOutput);
}