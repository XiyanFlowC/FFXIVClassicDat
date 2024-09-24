// FFXIVClassicDat.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _CRT_SECURE_NO_WARNINGS

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
#include "Config.h"

#include "FileScanner.h"
#include "SsdOperation.h"

//const std::wstring FFXIV_INSTALL_PATH = L"C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV";
// std::wstring FFXIV_INSTALL_PATH = L"C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV";

#include "SsdViewer.h"
#include <fstream>
#include "liteopt.h"

#include "RunAsAdmin.h"

int config(const char *para)
{
    std::string p(para);
    std::string key = p.substr(0, p.find_first_of('=')), value = p.substr(p.find_first_of('=') + 1);
    if (key == "ffxiv-install-location")
        Config::GetInstance().m_ffxivInstallPath = xybase::string::to_wstring(value);
    if (key == "output-location")
        Config::GetInstance().m_workArea = xybase::string::to_wstring(value);
    return 0;
}

int setInstallLocation(const char *para)
{
    Config::GetInstance().m_ffxivInstallPath = xybase::string::to_wstring(para);
    return 0;
}

int setWorkAreaLocation(const char *para)
{
    Config::GetInstance().m_workArea = xybase::string::to_wstring(para);
    return 0;
}

int setLanguage(const char *para)
{
    std::string p(para);
    int val = xybase::string::stoi(p);
    if (p == "ja") val = 0;
    else if (p == "en") val = 1;
    else if (p == "de") val = 2;
    else if (p == "fr") val = 3;
    else if (p == "chs") val = 4;
    else if (p == "cht") val = 5;

    if (val < 0 || val > 5)
    {
        return 10;
    }
    Config::GetInstance().m_lang = val;
    return 0;
}

int help(const char *para)
{
    std::wcout << L"FFXIV 1.0 数据文件处理控制台" << std::endl
        << std::endl;
    for (int i = 0; i < 32; ++i)
    {
        if (!LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_DESC_VLD)) continue;
        if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_CH_VLD))
            std::wcout << std::format(L"-{}  ", (wchar_t)_reged_opt[i].ch_opt);
        else
            std::wcout << L"    ";
        if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_STR_VLD))
            std::wcout << std::format(L"--{:16} ", xybase::string::to_wstring(_reged_opt[i].long_opt));
        else
            std::wcout << L"                  ";

        if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_VAL_NEED))
            std::wcout << L"* ";
        else
            std::wcout << L"  ";

        auto ch = (wchar_t *)_reged_opt[i].desc;
        int cur = 0;
        int lfc = 24;
        while (*ch)
        {
            if (*ch == L'\n')
            {
                cur = 0;
                if (*++ch)
                    std::wcout << L"\n                             ";
                else
                    std::wcout << std::endl;
                continue;
            }
            std::wcout << *ch++;
            if (cur++ > lfc)
            {
                cur = 0;
                if (*ch)
                    std::wcout << L"\n                             ";
                else
                    std::wcout << std::endl;
            }
        }
        if (cur) std::wcout << L"\n";
    }
    std::wcout << L"标记了 * 的项目，表示该开关需要后随一个参数。" << std::endl;
    return 0;
}

enum Action {
    ACT_INVALID,
    ACT_BUILD_DATABASE,
    ACT_DECRYPT_SSD,
    ACT_EXPORT_SHEETS,
    ACT_IMPORT_SHEETS,
    ACT_EXPORT_SPECIFIED_SSD,
    ACT_IMPORT_SPECIFIED_SSD,
    ACT_PLACEHOLDER
};

Action action = ACT_INVALID;

uint32_t ssdTarget = 0;
std::u8string sheetName;
bool fullExport = false, update = false, force = false, trace = false;

int main(int argc, const char ** argv)
{
    setlocale(LC_ALL, "");

    lopt_regopt("config", 'c', LOPT_FLG_VAL_NEED, config, nullptr);
    lopt_regopt("install-path", 'I', LOPT_FLG_VAL_NEED, setInstallLocation, L"设定安装目录。会记忆该设定。");
    lopt_regopt("work-area-path", 'O', LOPT_FLG_VAL_NEED, setWorkAreaLocation, L"设定工作区。会记忆该设定。");
    lopt_regopt("lang", 'L', LOPT_FLG_VAL_NEED, setLanguage, L"设定语言。会记忆该设定。有效值：[0,5]");
    
    lopt_regopt("scan", 'S', 0, [](const char *para)->int {
        std::filesystem::remove("type.txt");
        action = ACT_BUILD_DATABASE;
        return 0;
        }, L"扫描游戏 data 文件夹。记录已知类型的文件存储位置。");
    lopt_regopt("decrypt-ssd", 'd', 0, [](const char *para)->int {
        action = ACT_DECRYPT_SSD;
        return 0;
        }, L"解密已知的（登录在 type.txt 中的）SSD 文件到工作区。");
    lopt_regopt("export-ssd", 'e', 0, [](const char *para)->int {
        if (para)
        {
            action = ACT_EXPORT_SPECIFIED_SSD;
            std::string p(para);
            if (p.starts_with("0x")) ssdTarget = xybase::string::stoi(p.substr(2), 16);
            else ssdTarget = xybase::string::stoi(p);
            return 0;
        }
        action = ACT_EXPORT_SHEETS;
        return 0;
        }, L"导出SSD。可以提供一个 Data ID 来指定要对哪一个 SSD 操作。不指定的情况下导出\ntype.txt 中指明的全部 SSD。");
    lopt_regopt("import-ssd", 'i', 0, [](const char *para)->int {
        if (para)
        {
            action = ACT_IMPORT_SPECIFIED_SSD;
            std::string p(para);
            if (p.starts_with("0x")) ssdTarget = xybase::string::stoi(p.substr(2), 16);
            else ssdTarget = xybase::string::stoi(p);
            return 0;
        }
        action = ACT_IMPORT_SHEETS;
        return 0;
        }, L"导入SSD。可以提供一个 Data ID 来指定要对哪一个 SSD 操作。不指定的情况下导入\ntype.txt 中指明的全部 SSD。");
    lopt_regopt("sheet", 's', LOPT_FLG_VAL_NEED, [](const char *para)->int {
        sheetName = xybase::string::to_utf8(para);
        return 0;
        }, L"指定要对哪一个表操作。SSD 导入导出时有效。");
    lopt_regopt("full-export", '\0', 0, [](const char *para) ->int {
        fullExport = true;
        return 0;
        }, L"无视 enable 进行 SSD 的导出。仅导出\nSSD 时有效。一般不需要设置。");
    lopt_regopt("update", 'U', 0, [](const char *para) ->int {
        update = true;
        return 0;
        }, L"导入时执行更新操作。合并现有数据和工作区中 CSV 的数据。");
    lopt_regopt("force", 'f', 0, [](const char *para)->int {
        force = true;
        return 0;
        }, L"强制导出。覆盖工作区中已经存在之前导出的数据。");
    lopt_regopt("trace-dat-access", 'T', 0, [](const char *para)->int {
        trace = true;
        return 0;
        }, L"记录数据文件访问情况。诊断时很有用。");
    lopt_regopt("help", '?', 0, help, L"显示本信息。");
    if (argc == 1) help(nullptr);


    int ret = lopt_parse(argc, argv);
    if (ret)
    {
        if (ret < 0)
        {
            std::wcerr << std::format(L"{}: 语法有误或是不存在的开关。\n", xybase::string::to_wstring(argv[-ret]));
        }
        if (ret == 10)
        {
            std::wcerr << L"-L: 错误，给定的值不在许可范围内（ja/en/de/fr/chs/cht）" << std::endl;
        }
        exit(ret);
    }

    lopt_finalize();

    try
    {
        DataManager::GetInstance().m_basePath = Config::GetInstance().GetGamePath() / "data";
        DataManager::GetInstance().m_traceAccess = trace;
        FileScanner fs;
        SsdOperation so;
        so.m_update = update;
        so.m_fullExport = fullExport;
        so.m_force = force;
        switch (action)
        {
        case ACT_INVALID:
            break;
        case ACT_BUILD_DATABASE:
            fs.FileScan();
            break;
        case ACT_DECRYPT_SSD:
            so.DecryptSsd();
            break;
        case ACT_EXPORT_SHEETS:
            so.ExportAllSsd(Config::GetInstance().GetWorkAreaPath() / "sheet" / Config::GetInstance().GetLangName(), sheetName);
            break;
        case ACT_IMPORT_SHEETS:
            so.ImportAllSsd(Config::GetInstance().GetWorkAreaPath() / "sheet" / Config::GetInstance().GetLangName(), sheetName);
            break;
        case ACT_EXPORT_SPECIFIED_SSD:
        {
            SsdData sd(ssdTarget, Config::GetInstance().GetLangName());
            so.ExportSsd(Config::GetInstance().GetWorkAreaPath() / "sheet" / Config::GetInstance().GetLangName(), sd, sheetName);
        }
        break;
        case ACT_IMPORT_SPECIFIED_SSD:
        {
            SsdData sd(ssdTarget, Config::GetInstance().GetLangName());
            so.ImportSsd(Config::GetInstance().GetWorkAreaPath() / "sheet" / Config::GetInstance().GetLangName(), sd, sheetName);
        }
        break;
        case ACT_PLACEHOLDER:
            break;
        default:
            break;
        }
    }
    catch (xybase::IOException &ex)
    {
        if (ex.GetErrorCode() == EACCES)
        {
            std::wcerr << L"权限不足，请求提权。" << std::endl;
            RunAsAdmin::Execute();
            return EACCES;
        }
        else std::wcerr << L"发生了异常。" << ex.GetErrorCode() << " - " << ex.GetMessage() << std::endl;
        exit(ex.GetErrorCode());
    }
#ifdef NDEBUG
    catch (xybase::RuntimeException &ex)
    {
        std::wcerr << L"发生了异常。" << ex.GetErrorCode() << " - " << ex.GetMessage() << std::endl;
        std::cin.get();
        exit(ex.GetErrorCode());
    }
    catch (xybase::Exception &ex)
    {
        std::wcerr << L"发生了严重的故障。" << ex.GetErrorCode() << " - " << ex.GetMessage() << std::endl;
    }
#endif
    
    // 似乎是用于可视化二进制数据的？不确定用途：var_equip var_tex_path var_wep
    // 若以文本形式保存这些表可能需要10G以上的空间
    // 价值低，放弃
    //SsdData debugSsd(0x3A70000, LANG); // CDev.Engine.Fw.Framework.Debug
    //std::cout << "debug SSD" << std::endl;
    //ent = exportBase / "debug" / LANG;
    //exportSsd(ent, debugSsd);

    return 0;
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
