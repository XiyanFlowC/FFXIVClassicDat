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

const std::wstring FFXIV_INSTALL_PATH = L"C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV";

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
            std::cerr << std::format("File missing! Data Id {:08X}, Ssd: {}\n", ex.GetFileId(), (char *)entry->GetName().c_str());
            std::ofstream expen("Missing Dat.txt", std::ios::app);
            expen << std::format("{} : {}({:08X})\n", (char *)entry->GetName().c_str(), ex.GetFileId(), ex.GetFileId());
            expen.close();
            continue;
        }
        if (!std::filesystem::exists(path.parent_path())) std::filesystem::create_directories(path.parent_path());
        std::ofstream pen(path);
        pen << (char *)entry->ToCsv().c_str();
        pen.close();
        std::cout << "Exported.\n";
    }
}

/**
 * @brief 导出Sqwt的SSD。这些是界面UI文字。
 */
void exportSqwtSsdFiles()
{
    DataManager::GetInstance().m_basePath = FFXIV_INSTALL_PATH + L"\\data";
    const std::u8string LANG = u8"chs"; // 有效值：ja en de fr chs cht 日 英 德 法 汉（简 繁）

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
}

#include "ShuffleString.h"
#include "SqwtDecryptUtility.h"

/**
 * @brief 
 * @param bd 
 * @return 0 - not xml 1 - xml 2 - ssd
 */
int GetXmlType(BinaryData &bd)
{
    if (memcmp(bd.GetData(), "\xEF\xBB\xBF", 3) == 0)
    {
        std::string str((char *)bd.GetData(), 3, bd.GetLength() - 3);
        if (str.starts_with("<?xml"))
        {
            if (str.find("<ssd ") != std::string::npos)
            {
                return 2;
            }
            return 1;
        }
    }

    ShuffleString ss;
    BinaryData tmp(bd.GetLength() + 1);
    int length = ss.Decrypt(bd.GetData(), bd.GetLength(), tmp.GetData(), tmp.GetLength());
    if (length < 0)
    {
        length = bd.GetLength();
    }
    ((char *)tmp.GetData())[length] = 0;

    if (memcmp(tmp.GetData(), "\xEF\xBB\xBF", 3) == 0)
    {
        std::string str((char *)tmp.GetData() + 3);
        if (str.starts_with("<?xml"))
        {
            if (str.find("<ssd ") != std::string::npos)
            {
                return 2;
            }
            return 1;
        }
    }
    return 0;
}

int IsOgg(BinaryData &bd)
{
    return 0;
}

int IsBlock(BinaryData &bd)
{
    return 0;
}

int IsGtex(BinaryData &bd)
{
    return 0;
}

void FileDetect(std::ofstream &recorder, const std::filesystem::directory_entry &ent)
{
    std::cout << "Regular file " << ent.path();

    size_t size = ent.file_size();
    uint8_t *buffer = new uint8_t[size];
    std::ifstream eye(ent.path(), std::ios::binary);
    eye.read((char *)buffer, size);
    eye.close();
    BinaryData bd(buffer, size, false);

    if (int x = GetXmlType(bd))
    {
        if (x == 1)
        {
            recorder << "XML" << "\t" << ent.path() << std::endl;
            std::cout << "XML!\n";
        }
        else if (x == 2)
        {
            recorder << "SSD" << "\t" << ent.path() << std::endl;
            std::cout << "SSD!\n";
        }
    }
    else if (IsOgg(bd))
    {
        recorder << "OGG" << "\t" << ent.path() << std::endl;
        std::cout << "OGG!\n";
    }
    else if (IsGtex(bd))
    {
        recorder << "GTEX" << "\t" << ent.path() << std::endl;
        std::cout << "GTEX!\n";
    }
    else
    {
        std::cout << "???\n";
    }
}

void FileScan()
{
    std::filesystem::path dataPath = FFXIV_INSTALL_PATH + L"\\data";
    //std::filesystem::path dataPath = ;
    std::filesystem::recursive_directory_iterator itr(dataPath);
    std::ofstream recorder("type.txt");
    // FileDetect(recorder, std::filesystem::directory_entry("C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV\\data\\0B\\45\\0B\\C8.DAT"));
    for (auto &&ent : itr)
    {
        if (ent.is_regular_file())
        {
            FileDetect(recorder, ent);
        }
    }
}

void DecryptSsd()
{
    std::ifstream record("type.txt");

    while (record)
    {
        std::string type;
        std::filesystem::path path;
        record >> type >> path;
        if (type == "SSD")
        {
            std::ifstream f(path, std::ios::binary);
            std::filesystem::path outputPath(path.lexically_relative(FFXIV_INSTALL_PATH));
            std::filesystem::create_directories(outputPath.parent_path());
            std::ofstream o(outputPath, std::ios::binary);
            size_t size = std::filesystem::file_size(path);
            char *buffer = new char[size];
            f.read(buffer, size);
            ShuffleString ss;
            int ret = ss.Decrypt(buffer, size, buffer, size);
            if (ret < 0) ret = size;
            f.close();
            o.write(buffer, ret);
            o.close();

            delete []buffer;
        }
    }
}

int main()
{
    setlocale(LC_ALL, "");

    // exportSqwtSsdFiles();

    // FileScan();

    DecryptSsd();

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
