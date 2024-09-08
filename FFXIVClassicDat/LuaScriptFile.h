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
	 * @brief ���ݸ�����·������ѯstaticactor�����ܺ��ȡʵ���ļ������н��ܺ󣬷������ĵ���Luac���롣
	 * ��ʹ��Unluac�Ƚ�һ������
	 */
	BinaryData GetLuacDataByName(const std::wstring &path);

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

