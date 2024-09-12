#pragma once
#include <cstdint>
#include <string>

class BinaryData;

/**
 * @brief 用于处理Sqwt相关文件的类
 */
class SqwtFile
{
	const char SQEX_MAGIC_HEAD[4] = { 'S', 'Q', 'E', 'X' };
public:
	BinaryData FileContent;

	struct SqwtFileHeader
	{
		char magicHeader;
		uint32_t a;
	};

	SqwtFile(std::wstring path);

	/**
	 * @brief 载入文件到内存，若文件已被加密则首先解密。
	 * @param path 要载入的文件名
	 */
	void LoadFile(std::wstring path);

	void ParseFile();
};

