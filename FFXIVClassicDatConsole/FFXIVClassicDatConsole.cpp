// FFXIVClassicDat.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <clocale>

#include "SsdData.h"
#include "DataManager.h"
#include "SqwtFile.h"
#include "Sheet.h"
#include "xybase/xystring.h"

void exportSsd(const std::filesystem::path &p_path, const SsdData &p_ssd)
{
    for (auto &&entry : p_ssd.GetAllSheets())
    {
        std::cout << xybase::string::to_string(entry->GetName()) << ": ";
        auto path = p_path / (entry->GetName() + u8".csv");
        if (std::filesystem::exists(path))
        {
            std::cout << xybase::string::to_string(L"Skip.\n");
            continue;
        }
        std::cout << "Loading...";
        try
        {
            entry->LoadAll();
        }
        catch (DataManager::FileMissingException &ex)
        {
            std::cerr << std::format("File missing! Data Id {:08X}\n", ex.GetFileId());
            continue;
        }
        if (!std::filesystem::exists(path.parent_path())) std::filesystem::create_directories(path.parent_path());
        std::ofstream pen(path);
        pen << (char *)entry->ToCsv().c_str();
        pen.close();
        std::cout << "Exported.\n";
    }
}

int main()
{
    setlocale(LC_ALL, "");

    const std::wstring FFXIV_INSTALL_PATH = L"C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV";
    DataManager::GetInstance().m_basePath = FFXIV_INSTALL_PATH + L"\\data";
    const std::u8string LANG = u8"ja"; // 有效值：ja en de fr chs cht 日 英 德 法 汉（简 繁）

    // Sqex Sqwt 分析管理器根据程序不同自动切换？
    // 程序      Ssd          初始化的sqwt基路径
    // Boot   0x27950000   client\\sqwt\\boot\\
    // Game   0x01030000   client\\sqwt\\ 
    SsdData bootSsd(0x27950000, LANG);
    SsdData gameSsd(0x01030000, LANG);

    std::cout << "ffxivboot.exe SSD" << std::endl;
    std::filesystem::path exportBase("sheets");
    std::filesystem::path ent = exportBase / "ffxivboot" / LANG;
    exportSsd(ent, bootSsd);

    std::cout << "ffxivgame.exe SSD" << std::endl;
    ent = exportBase / "ffxivgame" / LANG;
    exportSsd(ent, gameSsd);

    // 似乎是用于可视化二进制数据的？不确定用途：var_equip var_tex_path var_wep
    // 若以文本形式保存这些表可能需要10G以上的空间
    // 价值低，放弃
    //SsdData debugSsd(0x3A70000, LANG); // CDev.Engine.Fw.Framework.Debug
    //std::cout << "debug SSD" << std::endl;
    //ent = exportBase / "debug" / LANG;
    //exportSsd(ent, debugSsd);

    // std::wstring SQWT_BASE_PATH = FFXIV_INSTALL_PATH + L"\\client\\sqwt\\boot";

    // SqwtFile candidateList(SQWT_BASE_PATH + L"\\system\\ime\\CandidateList.form");
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
