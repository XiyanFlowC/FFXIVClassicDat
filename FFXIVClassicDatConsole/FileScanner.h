#pragma once
#include <filesystem>
#include "Config.h"

class BinaryData;

class FileScanner
{
public:
    /**
     * @brief ɨ�������ļ��У��²��ļ�����
     */
    void FileScan();
protected:
    /**
     * @brief
     * @param bd
     * @return 0 - not xml 1 - xml 2 - ssd
     */
    int GetXmlType(BinaryData &bd);

    // TODO: �����������ļ�
    int IsOgg(BinaryData &bd);

    int IsBlock(BinaryData &bd);

    int IsGtex(BinaryData &bd);

    void FileDetect(std::ofstream &recorder, const std::filesystem::directory_entry &ent);
};

