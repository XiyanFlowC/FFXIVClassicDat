#include "LuaScriptFile.h"

#include "xybase/xystring.h"

std::wstring LuaScriptFile::subalphabet = L"0123456789abcdefghijklmnopqrstuvwxyz";
std::wstring LuaScriptFile::revalphabet = L"jihgfedcba9876543210zyxwvutsrqponmlk";

std::wstring LuaScriptFile::FileNameCipher(std::wstring_view p_fileName)
{
	std::wstring ret;
	std::wstring fileName = xybase::string::to_lower(std::wstring{ p_fileName });
	for (auto &&ch : fileName)
	{
		size_t code = subalphabet.find(ch);
		if (code != std::wstring::npos)
		{
			ret += revalphabet[code];
		}
		else
			ret += ch;
	}
}

LuaScriptFile::LuaScriptFile(const std::wstring &p_staticActorSanPath)
{
}

std::wstring LuaScriptFile::FileNameDecipher(std::wstring_view p_fileName)
{
	std::wstring ret;
	for (auto &&ch : p_fileName)
	{
		size_t code = revalphabet.find(ch);
		if (code != std::wstring::npos)
		{
			ret += subalphabet[code];
		}
		else
			ret += ch;
	}
}
