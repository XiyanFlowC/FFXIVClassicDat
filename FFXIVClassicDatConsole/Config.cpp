#include "Config.h"

#include <xybase/BinaryStream.h>
#include <xybase/xystring.h>
#include <string>
#include <format>
#include <iostream>

Config::Config()
{
	try
	{
		xybase::BinaryStream config(L"config.bin", L"rb");
		if (config.ReadInt32() != 1)
		{
			config.Close();
			return;
		}
		m_lang = config.ReadInt32();
		m_ffxivInstallPath = xybase::string::to_wstring(config.ReadString());
		m_workArea = xybase::string::to_wstring(config.ReadString());
	}
	catch (xybase::IOException)
	{
		// do noting
	}
}

Config::~Config()
{
	xybase::BinaryStream config(L"config.bin", L"wb");
	config.Write((int32_t)1);
	config.Write(m_lang);
	config.Write(xybase::string::to_string(m_ffxivInstallPath));
	config.Write(xybase::string::to_string(m_workArea));
}

Config &Config::GetInstance()
{
	static Config _inst;
	return _inst;
}

std::u8string Config::GetLangName()
{
	switch (m_lang)
	{
	case 0:
		return u8"ja";
	case 1:
		return u8"en";
	case 2:
		return u8"de";
	case 3:
		return u8"fr";
	case 4:
		return u8"chs";
	case 5:
		return u8"cht";
	default:
		return u8"";
	}
}

#include "LocLite.h"

std::filesystem::path Config::GetWorkAreaPath() const
{
	return std::filesystem::path(m_workArea);
}

std::filesystem::path Config::GetGamePath() const
{
	return std::filesystem::path(m_ffxivInstallPath);
}

void Config::Interface()
{
	const wchar_t *(tbl[])[6] = {
		{L"コンフィグは変更されました。再起動してください。\n", L"Config changed. Please restart the programe.\n", nullptr, nullptr, L"配置已改变。请重新启动程序。\n", nullptr},
		{
			L"コンフィグ：\n１：ファイナルファンタジーＸＩＶのインストールパスの設定\n２：ワークエリアパスの設定\n３：言語設定\n",
			L"Config: 1- set install folder\n 2 - set work folder\n 3 - set language.\n", 
			L"Konfiguration: 1: Installationsordner festlegen\n2: Arbeitsordner festlegen\n3: Sprache festlegen.\n",
			L"Config:\n1: définir le dossier d'installation\n2: définir le dossier de travail\n3: définir la langue.\n",
			L"设定：\n1：设置最终幻想XIV的安装目录\n2：设置软件工作目录\n3：设置语言\n",
			L"設定：\n1：設定太空戰士XIV的安裝目錄\n2：設定軟體工作目錄\n3：設定語言\n"
		}, {
			L"現在の設定：\nFFXIVのインストールパス：{}\nワーくエリアパス：{}\nデフォルト言語：{}\n",
			L"Current Settings:\nFFXIV Install Folder: {}\nWork Area: {}\nDefault Language: {}\n", nullptr, nullptr,
			L"现在设定：\nFFXIV安装目录：{}\n工作目录：{}\n默认处理语言：{}\n", nullptr
		}, {
			L"無効なパラメータ。", L"Invalid Parameter.", nullptr, L"Paramètre invalide.", L"无效参数。", nullptr
		},
		{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	};
	SetTTable(tbl);
	int cmd = -1;
	while (cmd != 0)
	{
		std::wcout << std::vformat(I(2), std::make_wformat_args(Config::GetInstance().m_ffxivInstallPath, Config::GetInstance().m_workArea, 
			xybase::string::to_wstring(Config::GetInstance().GetLangName())));
		std::wcout << I(1);
		std::wcout << "Config? ";
		std::wcin >> cmd;

		if (cmd == 1)
		{
			std::wcout << "Config>FF14 Install Path? ";
			std::getline(std::wcin, Config::GetInstance().m_ffxivInstallPath);
			std::getline(std::wcin, Config::GetInstance().m_ffxivInstallPath);
		}
		else if (cmd == 2)
		{
			std::wcout << "Config>Work Area? ";
			std::getline(std::wcin, Config::GetInstance().m_workArea);
			std::getline(std::wcin, Config::GetInstance().m_workArea);
		}
		else if (cmd == 3)
		{
			std::wcout << "0-ja 1-en 2-de 3-fr 4-chs 5-cht\n";
			std::wcout << "Config>Language? ";
			std::wcin >> cmd;
			if (cmd > 5 && cmd < 0)
			{
				std::wcout << I(3);
			}
			else Config::GetInstance().m_lang = cmd;
			cmd = -1;
		}
	}
	std::wcout << I(0);
	while (1)
	{
		std::wcin >> cmd;
		std::wcout << I(0);
		exit(0);
	}
}
