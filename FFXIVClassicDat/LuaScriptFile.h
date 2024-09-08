#pragma once

#include <string>
#include "BinaryData.h"
#include <map>

class LuaScriptFile
{
	static std::wstring subalphabet;
	static std::wstring revalphabet;
protected:
	std::map<uint32_t, std::u8string> actors;
	std::wstring basePath;

public:
	struct SanHeader
	{
		char magic[4];
		char ukn[9];
	};

	struct Actor
	{
		uint32_t id;
		char name[1]; // 标志用，长度变化
	};

	struct LpdHeader
	{
		char magic[4];
		uint32_t version;
		uint32_t fileSize;
		char ukn;
		char script[1];
	};

	/**
	 * @brief 初始化脚本访问工具。需给出脚本基本路径。路径下需要描述脚本对应关系的
	 * staticactor.san存在才可正常运作。
	 */
	LuaScriptFile(const std::wstring &p_scriptBasePath);


	BinaryData GetLuacDataByPath(const std::wstring &path);

	/**
	 * @brief 根据给出的Actor名，查询staticactor，加密路径后获取实际文件。进行解密后，返回明文的裸Luac代码。
	 * 可使用Unluac等进一步处理
	 */
	BinaryData GetLuacDataByName(const std::u8string &actorName);

	/**
	 * @brief 使用ActorId获取文件
	 * @note 该ActorId和游戏内部Id对应关系未确认
	 * @param id 
	 * @return 
	 */
	BinaryData GetLuacDataByActorId(uint32_t id);

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

