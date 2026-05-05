#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

enum class FileType
{
    Unknown,
    FDT,       // VERS magic — font data table
    GTEX,      // GTEX magic — texture
    SSD,       // XML with <ssd> tag — spreadsheet data
    XML,       // plain XML (not SSD)
    DDS,       // DDS texture file
    SEDB,      // SEDB magic — sound effect database
    SQWT,      // SQEX magic — encrypted UI XML
    ZiPatch,   // 0x91 ZIPATCH — patch file
    CFB,       // Microsoft Compound File Binary (.xls etc.)
    VGRD,      // VfxGraphResourceData
    MLRD,      // MapLayoutResourceData
    Binary     // fallback — display as hex
};

std::wstring_view FileTypeName(FileType type);

FileType DetectFileType(const uint8_t* data, size_t size);

struct FileTypeResult
{
    FileType type = FileType::Binary;
    int xmlSubType = 0;  // 0=none, 1=xml, 2=ssd
};

FileTypeResult DetectFileTypeDetail(const uint8_t* data, size_t size);
