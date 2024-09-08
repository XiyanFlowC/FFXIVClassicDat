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
		char name[1]; // ��־�ã����ȱ仯
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
	 * @brief ��ʼ���ű����ʹ��ߡ�������ű�����·����·������Ҫ�����ű���Ӧ��ϵ��
	 * staticactor.san���ڲſ�����������
	 */
	LuaScriptFile(const std::wstring &p_scriptBasePath);


	BinaryData GetLuacDataByPath(const std::wstring &path);

	/**
	 * @brief ���ݸ�����Actor������ѯstaticactor������·�����ȡʵ���ļ������н��ܺ󣬷������ĵ���Luac���롣
	 * ��ʹ��Unluac�Ƚ�һ������
	 */
	BinaryData GetLuacDataByName(const std::u8string &actorName);

	/**
	 * @brief ʹ��ActorId��ȡ�ļ�
	 * @note ��ActorId����Ϸ�ڲ�Id��Ӧ��ϵδȷ��
	 * @param id 
	 * @return 
	 */
	BinaryData GetLuacDataByActorId(uint32_t id);

	/**
	 * @brief ���ܽű��ļ��������򵥵Ĵ�������
	 * @param p_fileName �ļ�������
	 * @return �ļ�������
	 */
	static std::wstring FileNameDecipher(std::wstring_view p_fileName);

	/**
	 * @brief �Խű��ļ������м���
	 * @param p_fileName �ļ�������
	 * @return �ļ�������
	 */
	static std::wstring FileNameCipher(std::wstring_view p_fileName);
};

