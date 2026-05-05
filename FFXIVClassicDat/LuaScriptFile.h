#pragma once

#include <string>
#include <map>

class BinaryData;

class LuaScriptFile
{
	static std::wstring s_subAlphabet;
	static std::wstring s_revAlphabet;
protected:
	std::map<uint32_t, std::u8string> m_actors;
	std::wstring m_basePath;

public:
	struct SanHeader
	{
		char magic[4];
		char ukn[8];
		// following is Actors, maybe encrypted by simplestring
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
		char script[1]; // script data, may be encrypted by simplestring
	};

	/**
	 * @brief 初始化脚本访问工具。需给出脚本基本路径。路径下需要描述脚本对应关系的
	 * staticactor.san存在才可正常运作。
	 * @param scriptBasePath 脚本基本路径（如"client/script/"），末尾需要包含斜杠。
	 */
	LuaScriptFile(const std::wstring &scriptBasePath);

	BinaryData GetLuacDataByPath(const std::wstring &path);

	/**
	 * @brief 根据给出的Actor名，查询staticactor，加密路径后获取实际文件。进行解密后，返回明文的裸Luac代码。
	 * 可使用Unluac等进一步处理
	 * @param actorName Actor名称（如"Rhalgr"）
	 */
	BinaryData GetLuacDataByName(const std::u8string &actorName);

	/**
	 * @brief 使用ActorId获取文件
	 * @note 该ActorId和游戏内部Id对应关系未确认
	 * @param id Actor ID
	 * @return 解密后的Luac字节码
	 */
	BinaryData GetLuacDataByActorId(uint32_t id);

	/**
	 * @brief 解密脚本文件名。（简单的代替密码
	 * @param p_fileName 文件名密文
	 * @return 文件名明文
	 */
	static std::wstring FileNameDecipher(std::wstring_view fileName);

	/**
	 * @brief 对脚本文件名进行加密
	 * @param p_fileName 文件名明文
	 * @return 文件名密文
	 */
	static std::wstring FileNameCipher(std::wstring_view fileName);
};
