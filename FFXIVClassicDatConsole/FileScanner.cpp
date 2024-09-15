#include "FileScanner.h"

#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

#include "BinaryData.h"
#include "ShuffleString.h"
#include "SqwtDecryptUtility.h"

int FileScanner::GetXmlType(BinaryData &bd)
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

int FileScanner::IsOgg(BinaryData &bd)
{
    return 0;
}

int FileScanner::IsBlock(BinaryData &bd)
{
    return 0;
}

int FileScanner::IsGtex(BinaryData &bd)
{
    return 0;
}

void FileScanner::FileDetect(std::ofstream &recorder, const std::filesystem::directory_entry &ent)
{
    std::wcout << L"Regular file " << ent.path();

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
            std::wcout << L"XML!\n";
        }
        else if (x == 2)
        {
            recorder << "SSD" << "\t" << ent.path() << std::endl;
            std::wcout << L"SSD!\n";
        }
    }
    else if (IsOgg(bd))
    {
        recorder << "OGG" << "\t" << ent.path() << std::endl;
        std::wcout << L"OGG!\n";
    }
    else if (IsGtex(bd))
    {
        recorder << "GTEX" << "\t" << ent.path() << std::endl;
        std::wcout << L"GTEX!\n";
    }
    else
    {
        std::wcout << L"???\n";
    }
}

void FileScanner::FileScan()
{
    std::filesystem::path dataPath = Config::GetInstance().m_ffxivInstallPath + L"\\data";
    std::wcout << L"扫描目录：" << dataPath << std::endl;

    if (std::filesystem::exists("type.txt"))
    {
        std::wcout << L"找到了上一次的扫描结果。要再度扫描时，请先删除 type.txt。" << std::endl;
        return;
    }

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
