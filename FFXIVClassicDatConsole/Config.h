#pragma once

#include <string>
#include <filesystem>

class Config
{
	Config();

	~Config();

public:
	static Config &GetInstance();

	std::u8string GetLangName();

	int m_lang = 0;
	int m_displayLang = 1;
	std::wstring m_ffxivInstallPath = L"C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV";
	std::wstring m_workArea = L"./WorkArea/";

	std::filesystem::path GetWorkAreaPath() const;

	std::filesystem::path GetGamePath() const;

	void Interface();
};

