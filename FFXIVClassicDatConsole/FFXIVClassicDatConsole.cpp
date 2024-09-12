// FFXIVClassicDat.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>

#include "SsdData.h"
#include "DataManager.h"
#include "SqwtFile.h"
#include "xybase/xystring.h"

int main()
{
    const std::wstring FFXIV_INSTALL_PATH = L"C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV";
    DataManager::GetInstance().m_basePath = FFXIV_INSTALL_PATH + L"\\data";
    const std::u8string LANG = u8"chs"; // 有效值：ja en de fr chs cht 日 英 德 法 汉（简 繁）

    // Sqex Sqwt 分析管理器根据程序不同自动切换？
    // 程序      Ssd          初始化的sqwt基路径
    // Boot   0x27950000   client\\sqwt\\boot\\
    // Game   0x01030000   client\\sqwt\\ 
    SsdData bootSsd(0x27950000, LANG);
    SsdData gameSsd(0x01030000, LANG);

    std::cout << "ffxivboot.exe SSD" << std::endl;
    for (auto &&entry : bootSsd.GetAllSheets())
    {
        std::cout << xybase::string::to_string(entry->GetName()) << std::endl;
    }
    std::cout << "ffxivgame.exe SSD" << std::endl;
    for (auto &&entry : gameSsd.GetAllSheets())
    {
        std::cout << xybase::string::to_string(entry->GetName()) << std::endl;
    }

    SsdData debugSsd(0x3A70000, LANG); // CDev.Engine.Fw.Framework.Debug
    std::cout << "debug SSD" << std::endl;
    for (auto &&entry : debugSsd.GetAllSheets())
    {
        std::cout << xybase::string::to_string(entry->GetName()) << std::endl;
    }

    std::wstring SQWT_BASE_PATH = FFXIV_INSTALL_PATH + L"\\client\\sqwt\\boot";

    SqwtFile candidateList(SQWT_BASE_PATH + L"\\system\\ime\\CandidateList.form");
    // std::cout << (char*)patch.FileContent.GetData() << std::endl;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
