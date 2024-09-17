#include "FileScanner.h"

#include <cstring>
#include <string>
#include <iostream>
#include <fstream>

#include "BinaryData.h"
#include "ShuffleString.h"
#include "SqwtDecryptUtility.h"

#include <xybase/Xml/XmlNode.h>
#include <xybase/Xml/XmlParser.h>

const uint8_t MICROSOFT_COMPOUND_FILE_HEADER_SIGNATURE[] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};

std::string XmlOpenTagSimpleCheck(const char *buf, int length)
{
    const char *end = buf + length;

    if (*buf++ != '<')
    {
        return "";
    }
    
    std::string ret;
    while (*buf != ' ' && *buf != '\t' && *buf != '\r' && *buf != '\n' && *buf != '>')
    {
        if (buf >= end) return "";
        if ((*buf < 'a' || *buf > 'z') && (*buf < 'A' || *buf > 'Z') && *buf != '-' && *buf != ':') return "";
        ret += *buf++;
    }
    return ret;
}

int XmlCloseTagSimpleCheck(const char *buf, int length, const std::string &target)
{
    const char *end = buf + length - 1;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
    {
        --end;
    }
    if (*end != '>') return 0;
    
    auto t = target.rbegin();
    while (t != target.rend())
    {
        if (*t++ != *end--) return 0;
    }
    return 1;
}

int XmlTopLabelCheck(const char *buf, int length)
{
    auto tag = XmlOpenTagSimpleCheck(buf, length);
    if (tag == "") return 0;
    return XmlCloseTagSimpleCheck(buf, length, tag);
}

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
        if (XmlTopLabelCheck((char *)bd.GetData() + 3, bd.GetLength())) return 1;
    }
    else
    {
        if (memcmp(bd.GetData(), "<?xml", 5) == 0) return 1;
        if (XmlTopLabelCheck((char *)bd.GetData(), bd.GetLength())) return 1;
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
        std::string str((char *)tmp.GetData(), 3, tmp.GetLength() - 3);
        if (str.starts_with("<?xml"))
        {
            if (str.find("<ssd ") != std::string::npos)
            {
                return 2;
            }
            return 1;
        }
        if (XmlTopLabelCheck((char *)tmp.GetData() + 3, tmp.GetLength())) return 1;
    }
    else
    {
        if (memcmp(tmp.GetData(), "<?xml", 5) == 0) return 1;
        if (XmlTopLabelCheck((char *)tmp.GetData(), tmp.GetLength())) return 1;
    }
    return 0;
}

int FileScanner::IsOgg(BinaryData &bd)
{
    // Ogg 在 SEDB里，如此保存的 
    // 解析SEDB才可以抽出
    return 0;
}

int FileScanner::IsBlock(BinaryData &bd)
{
    return 0;
}

int FileScanner::IsGtex(BinaryData &bd)
{
    return memcmp(bd.GetData(), "GTEX", 4) == 0;
}

int FileScanner::IsVers(BinaryData &bd)
{
    // Vers file
    // int32 Magic; int32 ukn_a, ukn_b (count?); 
    // n files followed, starts with Type (char[4]) and a length.
    // e.g. 'VERS', 4i32, 4i32, ('GETX', x-u32, byte[x]){4}
    if (memcmp("VERS", bd.GetData(), 4) == 0) return 1;
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
    else if (IsVers(bd))
    {
        recorder << "VERS" << "\t" << ent.path() << std::endl;
        std::wcout << L"VERS!\n";
    }
    else if (memcmp("VfxGraphResourceData", bd.GetData(), 20) == 0)
    {
        // 某种索引文件？
        recorder << "VGRD" << "\t" << ent.path() << std::endl;
        std::wcout << L"VGRD!\n";
    }
    else if (memcmp("MapLayoutResourceData", bd.GetData(), 20) == 0)
    {
        // 某种索引文件？里面还塞了SEDB
        recorder << "MLRD" << "\t" << ent.path() << std::endl;
        std::wcout << L"MLRD!\n";
    }
    else if (memcmp("DDS ", bd.GetData(), 4) == 0)
    {
        recorder << "DDS" << "\t" << ent.path() << std::endl;
        std::wcout << L"DDS!\n";
    }
    // 不具备记录价值
    /*else if (memcmp("SEDB", bd.GetData(), 4) == 0)
    {
        recorder << "SEDB" << "\t" << ent.path() << std::endl;
        std::wcout << L"SEDB!\n";
    }*/
    else if (memcmp(MICROSOFT_COMPOUND_FILE_HEADER_SIGNATURE, bd.GetData(), 8) == 0)
    {
        // CFB, likely xls, doc, ppt, etc.
        // In this case, most likely is xls.
        recorder << "CFB" << "\t" << ent.path() << std::endl;
        std::wcout << L"CFB!\n";
    }
    else
    {
        std::wcout << L"???\n";
    }
}

void FileScanner::FileScan()
{
    std::filesystem::path dataPath = Config::GetInstance().GetGamePath() / L"data";
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
