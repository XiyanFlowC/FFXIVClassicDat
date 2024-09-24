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

    void ExportAllSsd(const std::filesystem::path &p_path, const std::u8string &p_sheet = u8"");

    void ImportAllSsd(const std::filesystem::path &p_path, const std::u8string &p_sheet = u8"");

    bool m_fullExport = false;
    bool m_recursive = true;
    bool m_update = false;
    bool m_force = false;

    /**
     * @brief ����Sqwt��SSD����Щ�ǽ���UI���֡�
     */
    // void exportSqwtSsdFiles();
};

