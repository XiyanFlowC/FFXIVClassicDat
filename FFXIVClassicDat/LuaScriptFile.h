#pragma once

#include <string>
#include "BinaryData.h"

class LuaScriptFile
{
	static std::wstring subalphabet;
	static std::wstring revalphabet;
public:

	LuaScriptFile(const std::wstring &p_staticActorSanPath);

	/**
	 * @brief 根据给出的路径，查询staticactor，加密后获取实际文件。进行解密后，返回明文的裸Luac代码。
	 * 可使用Unluac等进一步处理
	 */
	BinaryData GetLuacDataByName(const std::wstring &path);

	/**
	 * @brief 解密脚本文件名。（简单的代替密码
	 * @param p_fileName 文件名密文
	 * @return 文件名明文
	 */
	static std::wstring FileNameDecipher(std::wstring_view p_fileName);

	/**
	 * @brief 对脚本文件名进行加密
	 * @param p_fileName 文件名明文
	 * @return 文件名密文
	 */
	static std::wstring FileNameCipher(std::wstring_view p_fileName);
};

