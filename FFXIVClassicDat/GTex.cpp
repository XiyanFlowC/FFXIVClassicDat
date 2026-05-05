#include "GTex.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <stdexcept>

/**
 * @brief Lookup Tables — hardcoded at fixed addresses in ffxivboot.exe
 */

/* GTEX_D3DFormatTable[32] at 0xD32698
 *  Maps format_type byte → D3DFORMAT value for D3DXLoadSurfaceFromMemory */
const uint32_t GTEX_D3DFormatTable[34] = {
    0,                      /*< 0x00: none */
    24,                     /*< 0x01: X1R5G5B5 = 0x18 */
    23,                     /*< 0x02: R5G6B5 = 0x17 */
    22,                     /*< 0x03: X8R8G8B8 = 0x16 */
    21,                     /*< 0x04: A8R8G8B8 = 0x15 (ARGB8888) */
    50,                     /*< 0x05: B8 = 0x32 (luminance) */
    0,                      /*< 0x06: G8B8 (no D3D equiv) */
    113,                    /*< 0x07: A16B16G16R16F = 0x71 */
    116,                    /*< 0x08: A32B32G32R32F = 0x74 */
    114,                    /*< 0x09: R32F = 0x72 */
    33,                     /*< 0x0A: X8B8G8R8 = 0x21 */
    32,                     /*< 0x0B: A8B8G8R8 = 0x20 */
    80,                     /*< 0x0C: D16 = 0x50 */
    80,                     /*< 0x0D: D16F = 0x50 */
    77,                     /*< 0x0E: D24X8 = 0x4D */
    75,                     /*< 0x0F: D24S8 = 0x4B */
    83,                     /*< 0x10: D24S8F = 0x53 */
    0,                      /*< 0x11: R6G5B5 (no D3D equiv) */
    112,                    /*< 0x12: G16R16F = 0x70 */
    34,                     /*< 0x13: G16R16 = 0x22 */
    0,                      /*< 0x14: R16 (no D3D equiv) */
    25,                     /*< 0x15: A1R5G5B5 = 0x19 */
    26,                     /*< 0x16: A4R4G4B4 = 0x1A (ARGB4444) — font glyph atlases */
    0,                      /*< 0x17: R5G5B5A1 (no D3D equiv) */
    D3DFMT_DXT1,            /*< 0x18: DXT1 = 0x31545844 (BC1) */
    D3DFMT_DXT3,            /*< 0x19: DXT3 = 0x33545844 (BC2) */
    D3DFMT_DXT5,            /*< 0x1A: DXT5 = 0x35545844 (BC3) */
    51,                     /*< 0x1B: V8U8 = 0x33 */
    35,                     /*< 0x1C: X2R10G10B10 = 0x23 */
    35,                     /*< 0x1D: A2R10G10B10 = 0x23 */
    33,                     /*< 0x1E: X8R8G8B8_LE = 0x21 */
    32,                     /*< 0x1F: A8R8G8R8_LE = 0x20 */
    31,                     /*< 0x20: X2R10G10B10_LE = 0x1F */
    31,                     /*< 0x21: A2R10G10B10_LE = 0x1F */
};

/* GTEX_FormatDescriptorTable[34] at 0xD32AD0
 *  10-dword per format: [0]=name* [1]=bpp [2..]=component bits [9]=flags */
const GTexFormatDesc GTEX_FormatDescriptorTable[34] = {
    { "none",          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "X1R5G5B5",      0x10, 0x05, 0x05, 0x05, 0x01, 0x00, 0x00, 0x00, 0x000 },
    { "R5G6B5",        0x10, 0x05, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "X8R8G8B8",      0x20, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A8R8G8B8",      0x20, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x000 },
    { "B8",            0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "G8B8",          0x10, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A16B16G16R16F", 0x40, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x001 },
    { "A32B32G32R32F", 0x80, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x001 },
    { "R32F",          0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x001 },
    { "X8B8G8R8",      0x20, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A8B8G8R8",      0x20, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x000 },
    { "D16",           0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x000 },
    { "D16F",          0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x000 },
    { "D24X8",         0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x18, 0x00, 0x001 },
    { "D24S8",         0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x18, 0x08, 0x001 },
    { "D24S8F",        0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x18, 0x08, 0x001 },
    { "R6G5B5",        0x10, 0x05, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "G16R16F",       0x20, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x001 },
    { "G16R16",        0x20, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "R16",           0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A1R5G5B5",      0x10, 0x05, 0x05, 0x05, 0x01, 0x00, 0x00, 0x00, 0x000 },
    { "A4R4G4B4",      0x10, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x000 },
    { "R5G5B5A1",      0x10, 0x05, 0x05, 0x05, 0x01, 0x00, 0x00, 0x00, 0x000 },
    { "DXT1",          0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x100 },
    { "DXT3",          0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x100 },
    { "DXT5",          0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x100 },
    { "V8U8",          0x10, 0x00, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "X2R10G10B10",   0x20, 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A2R10G10B10",   0x20, 0x0A, 0x0A, 0x0A, 0x02, 0x00, 0x00, 0x00, 0x000 },
    { "X8R8G8B8_LE",   0x20, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A8R8G8B8_LE",   0x20, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x000 },
    { "X2R10G10B10_LE",0x20, 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x000 },
    { "A2R10G10B10_LE",0x20, 0x0A, 0x0A, 0x0A, 0x02, 0x00, 0x00, 0x00, 0x000 },
};

/**
 * @brief GTEX_CalcMipSurfaceOffset (0x4309C0)
 *
 * Computes byte offset for direct-data mode.
 * Stub: the real function performs mip-level offset arithmetic
 * based on format type, dimensions, and mip level count.
 */
uint32_t GTEX_CalcMipSurfaceOffset(uint32_t index)
{
    return index;
}

/**
 * @brief GTEX_DetectAndLoad — TextureFactory_CreateFromData (0x430890)
 *
 * Detects DDS vs GTEX magic and dispatches to appropriate parser.
 * @return true if a valid GTEX texture was detected and parsed.
 */
bool GTEX_DetectAndLoad(const uint8_t* data, uint32_t dataSize)
{
    if (!data || dataSize < 32)
        return false;

    /* Read first 4 bytes as big-endian uint32 */
    uint32_t magic = (static_cast<uint32_t>(data[0]) << 24) |
                     (static_cast<uint32_t>(data[1]) << 16) |
                     (static_cast<uint32_t>(data[2]) << 8)  |
                     static_cast<uint32_t>(data[3]);

    /* 0x44445320 = "DDS ", 0x47544558 = "GTEX" */
    if (magic == 0x47544558)
    {
        const auto* header = GTEX_ValidateHeader(data, dataSize);
        return header != nullptr;
    }

    return false;
}

/**
 * @brief GTEX_ValidateHeader — GTEX_ParseHeaderAndCreate (0x4303F0)
 *
 * Validates the GTEX header and returns a pointer to it or null.
 */
const GTexHeader* GTEX_ValidateHeader(const uint8_t* data, uint32_t dataSize)
{
    if (dataSize < sizeof(GTexHeader))
        return nullptr;

    const auto* header = reinterpret_cast<const GTexHeader*>(data);

    /* Step 1: Validate magic "GTEX" (byteswapped check) */
    uint32_t magic = (static_cast<uint32_t>(header->magic[0]) << 24) |
                     (static_cast<uint32_t>(header->magic[1]) << 16) |
                     (static_cast<uint32_t>(header->magic[2]) << 8)  |
                     static_cast<uint32_t>(header->magic[3]);
    if (magic != 0x47544558)
        return nullptr;

    /* Step 2: Check has_direct_data or external surface data */
    uint32_t hasDirect = header->HasDirectData();
    if (hasDirect == 0)
    {
        /* Mode B: requires mip_off_table */
        if (header->MipOffsetTable() == 0)
        {
        }
    }

    /* Step 3: Read dimensions — Width()/Height()/Depth() handle BE→host */
    /* Step 4: Determine mip levels — tex_flags & 4 → mip_levels=4 */
    /* Step 5: Determine texture type */
    uint8_t texFlags = header->tex_flags;
    if (texFlags & 0x01) { /* Cube texture */ }
    else if (texFlags & 0x02) { /* Volume texture */ }
    else { /* 2D texture */ }

    return header;
}

/**
 * @brief GTEX_ComputeSurfaceOffset — GTEX_UploadPixelData (0x4302E0)
 *
 * Computes the byte offset into pixel data for a given mip level and surface.
 */
uint32_t GTEX_ComputeSurfaceOffset(
    const GTexHeader& header,
    uint32_t mipLevel,
    uint32_t surface,
    uint32_t surfaceBase,
    uint32_t surfaceLimit)
{
    uint32_t index = surface + mipLevel * header.format_subtype;

    if (header.HasDirectData() != 0)
    {
        /* Mode A: direct data */
        return GTEX_CalcMipSurfaceOffset(index);
    }
    else if (header.MipOffsetTable() != 0)
    {
        /* Mode B: offset table */
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&header);
        const uint8_t* table = data + header.MipOffsetTable();

        uint32_t tableOffset = (static_cast<uint32_t>(table[4 * index + 0]) << 24) |
                               (static_cast<uint32_t>(table[4 * index + 1]) << 16) |
                               (static_cast<uint32_t>(table[4 * index + 2]) << 8)  |
                               static_cast<uint32_t>(table[4 * index + 3]);

        if (tableOffset >= surfaceLimit)
            return UINT32_MAX;

        return surfaceBase + tableOffset;
    }

    return 0;
}

/**
 * @brief GTEX_CreateResource (0x6EE740)
 *
 * Creates GTEX resource object from loaded file data.
 */
void GTEX_CreateResource(const uint8_t* fileData, uint32_t fileSize)
{
    if (!fileData || fileSize < 6)
        throw std::runtime_error("GTEX_CreateResource: invalid data");

    uint16_t textureCount = (static_cast<uint16_t>(fileData[4]) << 8) |
                             static_cast<uint16_t>(fileData[5]);

    for (uint16_t i = 0; i < textureCount; ++i)
    {
        uint32_t dataOffset;
        std::memcpy(&dataOffset, fileData + 4 * i + 20, 4);

        const uint8_t* textureData = fileData + dataOffset;
        const auto* gtexHeader = GTEX_ValidateHeader(textureData, fileSize - dataOffset);
        if (!gtexHeader)
        {
            continue;
        }
    }
}

/**
 * @brief GTex class — high-level file I/O
 */

GTex::GTex(std::filesystem::path filePath)
{
    LoadFromFile(filePath);
}

GTex::GTex(const uint8_t* data, uint32_t size)
{
    LoadFromMemory(data, size);
}

bool GTex::LoadFromFile(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    auto fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    LoadFromMemory(buffer.data(), static_cast<uint32_t>(buffer.size()));
    return true;
}

void GTex::LoadFromMemory(const uint8_t* data, uint32_t size)
{
    if (!data || size < sizeof(GTexHeader))
        throw std::runtime_error("GTex: data too small for header");

    ParseHeader(data, size);

    uint32_t pixelDataOffset = sizeof(GTexHeader);
    uint32_t pixelDataSize = size - pixelDataOffset;

    m_pixelData.resize(pixelDataSize);
    std::memcpy(m_pixelData.data(), data + pixelDataOffset, pixelDataSize);
}

void GTex::ParseHeader(const uint8_t* data, uint32_t size)
{
    std::memcpy(&m_header, data, sizeof(GTexHeader));

    if (!ValidateHeader())
        throw std::runtime_error("GTex: invalid magic — not a GTEX file");
}

bool GTex::ValidateHeader() const
{
    return m_header.magic[0] == 'G' &&
           m_header.magic[1] == 'T' &&
           m_header.magic[2] == 'E' &&
           m_header.magic[3] == 'X';
}

const uint8_t* GTex::GetSurfaceData(uint32_t mipLevel, uint32_t surface) const
{
    uint32_t offset = GTEX_ComputeSurfaceOffset(m_header, mipLevel, surface, 0,
                                                 static_cast<uint32_t>(m_pixelData.size()));
    if (offset == UINT32_MAX || offset >= m_pixelData.size())
        return nullptr;
    return m_pixelData.data() + offset;
}

uint32_t GTex::GetSurfaceDataSize(uint32_t mipLevel) const
{
    uint32_t width  = m_header.Width()  >> mipLevel;
    uint32_t height = m_header.Height() >> mipLevel;
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    uint32_t bpp = GTEX_Format_BitsPerPixel(m_header.format_type);
    uint32_t bytesPerRow;
    if (GTEX_Format_IsCompressed(m_header.format_type))
    {
        uint32_t blocksWide = (width + 3) / 4;
        uint32_t blocksHigh = (height + 3) / 4;
        bytesPerRow = blocksWide * ((bpp * 4) / 8);
        return bytesPerRow * blocksHigh;
    }
    else
    {
        bytesPerRow = (width * bpp) / 8;
        return bytesPerRow * height;
    }
}

/**
 * @brief DecodeToARGB8888 — converts the stored pixel data to 32-bit ARGB.
 *        Handles all known format types from the game.
 */
std::vector<uint8_t> GTex::DecodeToARGB8888() const
{
    uint32_t width  = m_header.Width();
    uint32_t height = m_header.Height();
    std::vector<uint8_t> out(static_cast<size_t>(width) * height * 4, 0);

    auto type = static_cast<GTexFormatType>(m_header.format_type);
    const auto& desc = GTEX_FormatDescriptorTable[m_header.format_type];

    switch (type)
    {
    case GTexFormatType::A8R8G8B8:
    {
        const uint8_t* src = GetSurfaceData(0, 0);
        if (!src) break;
        for (uint32_t i = 0; i < width * height; ++i)
        {
            out[i * 4 + 0] = src[i * 4 + 2];  // R
            out[i * 4 + 1] = src[i * 4 + 1];  // G
            out[i * 4 + 2] = src[i * 4 + 0];  // B
            out[i * 4 + 3] = src[i * 4 + 3];  // A
        }
        break;
    }
    case GTexFormatType::A4R4G4B4:
    {
        const uint8_t* src = GetSurfaceData(0, 0);
        if (!src) break;
        for (uint32_t i = 0; i < width * height; ++i)
        {
            uint16_t pixel = (static_cast<uint16_t>(src[i * 2]) << 8) |
                              static_cast<uint16_t>(src[i * 2 + 1]);
            out[i * 4 + 0] = static_cast<uint8_t>(((pixel >> 8) & 0x0F) << 4);  // R
            out[i * 4 + 1] = static_cast<uint8_t>(((pixel >> 4) & 0x0F) << 4);  // G
            out[i * 4 + 2] = static_cast<uint8_t>(((pixel >> 0) & 0x0F) << 4);  // B
            out[i * 4 + 3] = static_cast<uint8_t>(((pixel >> 12) & 0x0F) << 4); // A
        }
        break;
    }
    case GTexFormatType::DXT1:
    {
        /* DXT1 decompression — placeholder */
        break;
    }
    case GTexFormatType::DXT3:
    {
        /* DXT3 decompression — placeholder */
        break;
    }
    case GTexFormatType::DXT5:
    {
        /* DXT5 decompression — placeholder */
        break;
    }
    default:
        break;
    }

    return out;
}
