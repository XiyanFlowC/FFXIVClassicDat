#include "SsdOperation.h"

#include <fstream>

#include <DataManager.h>
#include <Sheet.h>
#include <xybase/xystring.h>
#include <ShuffleString.h>
#include "CsvUtility.h"

#include "Config.h"


void SsdOperation::ExportSheet(const std::filesystem::path &p_path, Sheet *p_sheet)
{
    std::wcout << xybase::string::to_wstring(p_sheet->GetName()) << ": ";
    auto path = p_path / (p_sheet->GetName() + u8".csv");
    if (!m_force && std::filesystem::exists(path))
    {
        std::wcout << xybase::string::to_wstring(L"Skip.\n");
        return;
    }
    std::wcout << L"Loading...";
    try
    {
        // 若需要完全导出，则不能忽略被enable禁用的项。且需要记录Enable
        p_sheet->m_cfgIgnoreEnableIndication = m_fullExport;
        p_sheet->LoadAll();
    }
    catch (DataManager::FileMissingException &ex)
    {
        std::wcerr << std::format(L"File missing! Data Id {:08X}, Ssd: {}\n", ex.GetFileId(), xybase::string::to_wstring(p_sheet->GetName()));
        std::ofstream expen("Missing Dat.txt", std::ios::app);
        expen << std::format("{} : {}({:08X})\n", (char *)p_sheet->GetName().c_str(), ex.GetFileId(), ex.GetFileId());
        expen.close();
        return;
    }
    if (!std::filesystem::exists(path.parent_path())) std::filesystem::create_directories(path.parent_path());
    std::wcout << "Outputing...";
    CsvFile csv(path.wstring(), CsvFile::OperationType::Write);
    p_sheet->SaveToCsv(csv);
    if (m_fullExport)
    {
        std::wcout << "Enable Outputing...";
        CsvFile enable(p_path / (p_sheet->GetName() + u8"_enable.csv"), CsvFile::OperationType::Write);
        p_sheet->EnableToCsv(enable);
    }
    std::wcout << "Exported.\n";
}

void SsdOperation::ImportSheet(const std::filesystem::path &p_path, Sheet *p_sheet)
{
    std::wcout << xybase::string::to_wstring(p_sheet->GetName()) << ": ";
    auto path = p_path / (p_sheet->GetName() + u8".csv");
    if (!std::filesystem::exists(path))
    {
        std::wcout << xybase::string::to_wstring(L"Skip.\n");
        return;
    }
    std::wcout << L"Loading...";
    
    try
    {
        // 更新模式：没有写明的行将保留原值，为此，首先载入原始文件
        if (m_update)
            p_sheet->LoadAll();
        CsvFile csv(path.wstring(), CsvFile::OperationType::Read);
        p_sheet->LoadFromCsv(csv);
    }
    catch (xybase::InvalidParameterException &ex)
    {
        std::wcerr << L"导入" << p_sheet->ToString() << L"发生错误。"
            << ex.GetErrorCode() << ex.GetMessage() << std::endl;
        return;
    }
    // 若存在Enable则需要导入
    if (std::filesystem::exists(p_path / (p_sheet->GetName() + u8"_enable.csv")))
    {
        std::wcout << "Enable Updating...";
        CsvFile enable((p_path / (p_sheet->GetName() + u8"_enable.csv")).wstring(), CsvFile::OperationType::Read);
        p_sheet->EnableFromCsv(enable);
    }
    
    if (!std::filesystem::exists(path.parent_path())) std::filesystem::create_directories(path.parent_path());
    std::wcout << "Saving...";
    p_sheet->SaveAll();
    std::wcout << "Imported.\n";
}

void SsdOperation::ExportSsd(const std::filesystem::path &p_path, const SsdData &p_ssd, const std::u8string &p_sheet)
{
    if (p_sheet != u8"")
    {
        auto sheet = p_ssd.GetSheet(p_sheet);
        if (sheet == nullptr) return;
        ExportSheet(p_path, sheet);
    }

    else for (auto &&entry : p_ssd.GetAllSheets())
    {
        if (entry->GetName() == u8"var_equip" || entry->GetName() == u8"var_wep") continue;
        ExportSheet(p_path, entry);
    }
}

void SsdOperation::ImportSsd(const std::filesystem::path &p_path, const SsdData &p_ssd, const std::u8string &p_sheet)
{
    if (p_sheet != u8"")
    {
        auto sheet = p_ssd.GetSheet(p_sheet);
        if (sheet == nullptr) return;
        ImportSheet(p_path, sheet);
    }

    else for (auto &&entry : p_ssd.GetAllSheets())
    {
        if (entry->GetName() == u8"var_equip" || entry->GetName() == u8"var_wep") continue;
        ImportSheet(p_path, entry);
    }
}

//void SsdOperation::exportSqwtSsdFiles()
//{
//    std::u8string LANG = Config::GetInstance().GetLangName();
//
//    // Sqex Sqwt 分析管理器根据程序不同自动切换？
//    // 程序      Ssd          初始化的sqwt基路径
//    // Boot   0x27950000   client\\sqwt\\boot\\ 
//    // Game   0x01030000   client\\sqwt\\ 
//    SsdData bootSsd(0x27950000, LANG);
//    SsdData gameSsd(0x01030000, LANG);
//
//    // std::cout << "ffxivboot.exe SSD" << std::endl;
//    std::filesystem::path exportBase("sheets");
//    std::filesystem::path ent = exportBase / "ffxivboot" / LANG;
//    exportSsd(ent, bootSsd);
//
//    // std::cout << "ffxivgame.exe SSD" << std::endl;
//    ent = exportBase / "ffxivgame" / LANG;
//    exportSsd(ent, gameSsd);
//}

void SsdOperation::DecryptSsd()
{
    if (!std::filesystem::exists("type.txt"))
    {
        std::wcout << "type.txt not found. Scan the data folder first.\n";
        std::wcout << L"找不到 type.txt。请先扫描数据文件夹。\n";
        return;
    }

    std::ifstream record("type.txt");

    while (record)
    {
        std::string type;
        std::filesystem::path path;
        record >> type >> path;
        if (type == "SSD")
        {
            std::ifstream f(path, std::ios::binary);
            std::filesystem::path outputPath = Config::GetInstance().GetWorkAreaPath() / path.lexically_relative(Config::GetInstance().m_ffxivInstallPath);
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

            delete[]buffer;
        }
    }
}

void SsdOperation::ExportAllSsd(const std::filesystem::path &p_path, const std::u8string &p_sheet)
{
    if (!std::filesystem::exists("type.txt"))
    {
        std::wcout << "type.txt not found. Scan the data folder first.\n";
        std::wcout << L"找不到 type.txt。请先扫描数据文件夹。\n";
        return;
    }

    std::ifstream record("type.txt");

    while (record)
    {
        std::string type;
        std::filesystem::path path;
        record >> type >> path;
        if (type == "SSD")
        {
            std::ifstream f(path, std::ios::binary);

            SsdData ssd(path, Config::GetInstance().GetLangName());
            // 禁止载入子项，避免重复操作
            ssd.m_recursive = false;
            ExportSsd(p_path, ssd, p_sheet);
        }
    }
}

void SsdOperation::ImportAllSsd(const std::filesystem::path &p_path, const std::u8string &p_sheet)
{
    if (!std::filesystem::exists("type.txt"))
    {
        std::wcout << "type.txt not found. Scan the data folder first.\n";
        std::wcout << L"找不到 type.txt。请先扫描数据文件夹。\n";
        return;
    }

    std::ifstream record("type.txt");

    while (record)
    {
        std::string type;
        std::filesystem::path path;
        record >> type >> path;
        if (type == "SSD")
        {
            std::ifstream f(path, std::ios::binary);

            SsdData ssd(path, Config::GetInstance().GetLangName());
            // 禁止载入子项，避免重复操作
            ssd.m_recursive = false;
            ImportSsd(p_path, ssd, p_sheet);
        }
    }
}
