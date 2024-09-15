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
		std::wcout << L"1-�趨�����Դ�Ŀ���ļ���\n2-�趨�����\n5 <n> <m>-�鿴n-m��\n6 <n> <m>-�鿴n��m�е����ݡ�\n9-��ָ�����Դ�Ŀ���ļ�\n" << std::endl;
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
			std::wcout << L"��һ���ļ������������ļ�ID����270B0000��" << std::endl;
			std::wcout << "SSD Viewer>Open? ";
			std::wcin >> file;
			try
			{
				m_ssd = new SsdData(std::stoi(file, nullptr, 16), Config::GetInstance().GetLangName());
			}
			catch (xybase::Exception &ex)
			{
				std::wcout << L"ָ����SSD�����Ѿ��𻵣����߲���SSD��" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}
			catch (xybase::RuntimeException &ex)
			{
				std::wcout << L"ָ����SSD�����Ѿ��𻵣����߲���SSD��" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}

			std::wcout << L"��SSD�������±�";
			for (auto &&sheet : m_ssd->GetAllSheets())
			{
				std::wcout << xybase::string::to_wstring(sheet->GetName()) << std::endl;
			}
		}
		if (cmd == 2)
		{
			if (m_ssd)
			{
				std::wcout << L"Ҫ����ı��ǣ�" << std::endl;
				std::wcout << "SSD Viewer>Load Sheet? ";
				std::wstring sheet;
				std::wcin >> sheet;
				if (m_sheet = m_ssd->GetSheet(xybase::string::to_utf8(sheet)))
				{
					std::wcout << L"ѡ����Ŀ�꣺" << m_sheet->ToString();
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
			std::wcout << L"��һ���ļ������������ļ�ID����270B0000��" << std::endl;
			std::wcout << "SSD Viewer>Open? ";
			std::wcin >> file;
			std::wcout << L"���ԣ�(ja/en/de/fr/chs/cht)" << std::endl;
			std::wcout << "SSD Viewer>Open? ";
			std::wcin >> lang;
			try
			{
				m_ssd = new SsdData(std::stoi(file, nullptr, 16), xybase::string::to_utf8(lang));
			}
			catch (xybase::Exception &ex)
			{
				std::wcout << L"ָ����SSD�����Ѿ��𻵣����߲���SSD��" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}
			catch (xybase::RuntimeException &ex)
			{
				std::wcout << L"ָ����SSD�����Ѿ��𻵣����߲���SSD��" << ex.GetErrorCode() << ex.GetMessage() << std::endl;
				continue;
			}
			std::wcout << L"��SSD�������±�";
			for (auto &&sheet : m_ssd->GetAllSheets())
			{
				std::wcout << xybase::string::to_wstring(sheet->GetName()) << std::endl;
			}
		}
	}
}
