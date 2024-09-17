#pragma once

#include <iostream>
#include <filesystem>
#include <SsdData.h>

class SsdOperation
{
public:
    void ExportSheet(const std::filesystem::path &p_dstPath, Sheet *p_sheet);

    void ImportSheet(const std::filesystem::path &p_dstPath, Sheet *p_sheet);

    void ExportSsd(const std::filesystem::path &p_path, const SsdData &p_ssd, const std::u8string &p_sheet = u8"");

    void ImportSsd(const std::filesystem::path &p_path, const SsdData &p_ssd, const std::u8string &p_sheet = u8"");

    void DecryptSsd();

    void ExportAllSsd(const std::filesystem::path &p_path);

    void ImportAllSsd(const std::filesystem::path &p_path);

    bool m_fullExport = false;

    /**
     * @brief 导出Sqwt的SSD。这些是界面UI文字。
     */
    // void exportSqwtSsdFiles();
};

