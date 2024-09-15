#include "SsdViewer.h"

#include <iostream>
#include <string>
#include "Config.h"
#include "SsdData.h"
#include <Sheet.h>
#include <xybase/Exception/InvalidParameterException.h>

SsdViewer::SsdViewer()
{
}

SsdViewer::~SsdViewer()
{
	if (m_ssd) delete m_ssd;
}

void SsdViewer::Interface()
{
	while (1)
	{
		std::wcout << L"1-设定并尝试打开目标文件。\n2-设定对象表\n5 <n> <m>-查看n-m行\n6 <n> <m>-查看n行m列的数据。\n9-以指定语言打开目标文件\n" << std::endl;
		std::wcout << "SSD Viewer? ";
		int cmd;
		std::wcin >> cmd;
		if (cmd == 0) break;

		if (cmd == 1)
		{
			if (m_ssd) delete m_ssd;
			if (m_sheet) delete m_sheet;
			m_ssd = nullptr;
			m_sheet = nullptr;

			std::wstring file;
			std::wcout << L"哪一个文件？（请输入文件ID，如270B0000）" << std::endl;
			std::wcout << "SSD Viewer>Open? ";
			std::wcin >> file;
			try
			{
				m_ssd = new SsdData(std::stoi(file, nullptr, 16), Config::GetInstance().GetLangName());
			}
			catch (xybase::Exception &ex)
			{
				std::wcout << L"指定的SSD可能已经损坏，或者不是SSD。" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}
			catch (xybase::RuntimeException &ex)
			{
				std::wcout << L"指定的SSD可能已经损坏，或者不是SSD。" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}

			std::wcout << L"该SSD包含以下表：";
			for (auto &&sheet : m_ssd->GetAllSheets())
			{
				std::wcout << xybase::string::to_wstring(sheet->GetName()) << std::endl;
			}
		}
		if (cmd == 2)
		{
			if (m_ssd)
			{
				std::wcout << L"要载入的表是？" << std::endl;
				std::wcout << "SSD Viewer>Load Sheet? ";
				std::wstring sheet;
				std::wcin >> sheet;
				if (m_sheet = m_ssd->GetSheet(xybase::string::to_utf8(sheet)))
				{
					std::wcout << L"选择了目标：" << m_sheet->ToString();
				}
			}
		}
		if (cmd == 5)
		{
			if (m_sheet)
			{
				int n, m;
				std::wcin >> n >> m;
				for (int i = n; i < m; ++i)
				{
					std::wcout << i << ':' << m_sheet->GetRow(i).ToString();
				}
			}
		}
		if (cmd == 6)
		{
			if (m_sheet)
			{
				int x, y;
				std::wcin >> x >> y;
				std::wcout << xybase::string::to_wstring(m_sheet->GetRow(x).GetCell(y).ToString()) << std::endl;
			}
		}
		if (cmd == 9)
		{
			if (m_ssd) delete m_ssd;
			m_ssd = nullptr;
			m_sheet = nullptr;

			std::wstring file, lang;
			std::wcout << L"哪一个文件？（请输入文件ID，如270B0000）" << std::endl;
			std::wcout << "SSD Viewer>Open? ";
			std::wcin >> file;
			std::wcout << L"语言？(ja/en/de/fr/chs/cht)" << std::endl;
			std::wcout << "SSD Viewer>Open? ";
			std::wcin >> lang;
			try
			{
				m_ssd = new SsdData(std::stoi(file, nullptr, 16), xybase::string::to_utf8(lang));
			}
			catch (xybase::Exception &ex)
			{
				std::wcout << L"指定的SSD可能已经损坏，或者不是SSD。" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}
			catch (xybase::RuntimeException &ex)
			{
				std::wcout << L"指定的SSD可能已经损坏，或者不是SSD。" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}
			std::wcout << L"该SSD包含以下表：";
			for (auto &&sheet : m_ssd->GetAllSheets())
			{
				std::wcout << xybase::string::to_wstring(sheet->GetName()) << std::endl;
			}
		}
	}
}
