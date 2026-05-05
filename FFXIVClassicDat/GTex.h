#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <filesystem>

/**
 * @brief GTEX Format Type Enum — matches the game's format_type byte at GTEX header +0x06
 */
enum class GTexFormatType : uint8_t
{
    NONE              = 0x00,
    X1R5G5B5          = 0x01,  /*< 16bpp */
    R5G6B5            = 0x02,  /*< 16bpp */
    X8R8G8B8          = 0x03,  /*< 32bpp */
    A8R8G8B8          = 0x04,  /*< 32bpp ARGB8888 */
    B8                = 0x05,  /*< 8bpp */
    G8B8              = 0x06,  /*< 16bpp */
    A16B16G16R16F     = 0x07,  /*< 64bpp float */
    A32B32G32R32F     = 0x08,  /*< 128bpp float */
    R32F              = 0x09,  /*< 32bpp float */
    X8B8G8R8          = 0x0A,  /*< 32bpp */
    A8B8G8R8          = 0x0B,  /*< 32bpp */
    D16               = 0x0C,  /*< depth 16 */
    D16F              = 0x0D,  /*< depth 16 float */
    D24X8             = 0x0E,  /*< depth 24+8 */
    D24S8             = 0x0F,  /*< depth 24+8 stencil */
    D24S8F            = 0x10,  /*< depth 24+8 float stencil */
    R6G5B5            = 0x11,  /*< 16bpp (nonstandard) */
    G16R16F           = 0x12,  /*< 32bpp float */
    G16R16            = 0x13,  /*< 32bpp */
    R16               = 0x14,  /*< 16bpp */
    A1R5G5B5          = 0x15,  /*< 16bpp */
    A4R4G4B4          = 0x16,  /*< 16bpp ARGB4444 — font glyph atlases */
    R5G5B5A1          = 0x17,  /*< 16bpp */
    DXT1              = 0x18,  /*< BC1 4bpp */
    DXT3              = 0x19,  /*< BC2 8bpp */
    DXT5              = 0x1A,  /*< BC3 8bpp */
    V8U8              = 0x1B,  /*< 16bpp normal map */
    X2R10G10B10       = 0x1C,  /*< 32bpp */
    A2R10G10B10       = 0x1D,  /*< 32bpp */
    X8R8G8B8LE        = 0x1E,  /*< 32bpp little-endian */
    A8R8G8B8LE        = 0x1F,  /*< 32bpp little-endian */
    X2R10G10B10LE     = 0x20,  /*< 32bpp little-endian */
    A2R10G10B10LE     = 0x21,  /*< 32bpp little-endian */
};

/**
 * @brief Texture type flags — byte at GTEX header +0x09
 */
enum class GTexTextureFlags : uint8_t
{
    NONE     = 0x00,
    CUBE     = 0x01,  /*< 6-face cube map */
    VOLUME   = 0x02,  /*< 3D volume texture (depth at +0x0E) */
    MIP_MAP  = 0x04,  /*< mipmapped (4 levels) */
};

/**
 * @brief GTEX Header — 32 bytes, all multi-byte fields BIG-ENDIAN
 *
 * Verified against ffxivboot.exe: sub_4303F0 (GTEX_ParseHeaderAndCreate)
 */
struct GTexHeader
{
    /** @brief offset 0x00: magic "GTEX" — 0x47 0x54 0x45 0x58 */
    char     magic[4];

    /** @brief offset 0x04: version — NEVER read by the game parser */
    uint8_t  version_hi;  /*< 0x01 */
    uint8_t  version_lo;  /*< 0x01 */

    /** @brief offset 0x06-0x07: format — read as TWO separate bytes */
    uint8_t  format_type;    /*< +0x06 → GTexFormatType, indexes lookup tables */
    uint8_t  format_subtype; /*< +0x07 → mip stride: offset_index = surface + mip * subtype */

    /** @brief offset 0x08: NEVER read by parser */
    uint8_t  unused_08;

    /** @brief offset 0x09: texture type flags (bitfield) */
    uint8_t  tex_flags; /*< GTexTextureFlags */

    /** @brief offset 0x0A-0x0D: dimensions (big-endian uint16) */
    uint8_t  width_be[2];   /*< +0x0A */
    uint8_t  height_be[2];  /*< +0x0C */

    /** @brief offset 0x0E: depth (big-endian uint16, only consumed for volume textures) */
    uint8_t  depth_be[2];

    /** @brief offset 0x10: mipmap offset table pointer (big-endian uint32)
     *  Points to an array of big-endian uint32 offsets relative to header start.
     *  Each offset table entry is 4 bytes (byte_swapped). Table length = face_count * surfaces. */
    uint8_t  mip_off_table[4];

    /** @brief offset 0x14: direct data flag (big-endian uint32, treated as boolean)
     *  Non-zero → pixel data starts immediately at +0x20 (Mode A)
     *  Zero     → pixel data accessed via mip_off_table (Mode B) */
    uint8_t  has_direct_data[4];

    uint8_t  reserved_18[4];  /*< offset 0x18: NEVER read */
    uint8_t  reserved_1C[4];  /*< offset 0x1C: NEVER read */

    /** @brief Helper accessors */
    uint16_t Width() const
    {
        return (static_cast<uint16_t>(width_be[0]) << 8) | width_be[1];
    }
    uint16_t Height() const
    {
        return (static_cast<uint16_t>(height_be[0]) << 8) | height_be[1];
    }
    uint16_t Depth() const
    {
        return (static_cast<uint16_t>(depth_be[0]) << 8) | depth_be[1];
    }
    uint32_t MipOffsetTable() const
    {
        return (static_cast<uint32_t>(mip_off_table[0]) << 24) |
               (static_cast<uint32_t>(mip_off_table[1]) << 16) |
               (static_cast<uint32_t>(mip_off_table[2]) << 8)  |
               static_cast<uint32_t>(mip_off_table[3]);
    }
    uint32_t HasDirectData() const
    {
        return (static_cast<uint32_t>(has_direct_data[0]) << 24) |
               (static_cast<uint32_t>(has_direct_data[1]) << 16) |
               (static_cast<uint32_t>(has_direct_data[2]) << 8)  |
               static_cast<uint32_t>(has_direct_data[3]);
    }
    bool IsCube()       const { return (tex_flags & 0x01) != 0; }
    bool IsVolume()     const { return (tex_flags & 0x02) != 0; }
    bool HasMipMaps()   const { return (tex_flags & 0x04) != 0; }
    uint32_t FaceCount() const { return IsCube() ? 6u : 1u; }
    uint32_t MipLevels() const { return HasMipMaps() ? 4u : 1u; }
};
static_assert(sizeof(GTexHeader) == 32, "GTexHeader must be 32 bytes");

/**
 * @brief Format Descriptor — mirrors GTEX_FormatDescriptorTable[type] (0xD32AD0)
 */
struct GTexFormatDesc
{
    const char* name;       /*< Name string */
    uint32_t bpp;           /*< Bits per pixel (GTEX_Format_BitsPerPixel) */
    uint32_t r_bits;        /*< Red component bit count */
    uint32_t g_bits;        /*< Green component bit count */
    uint32_t b_bits;        /*< Blue component bit count */
    uint32_t a_bits;        /*< Alpha component bit count */
    uint32_t depth_bits;    /*< Depth bits */
    uint32_t depth_int_bits;/*< Depth integer bits */
    uint32_t stencil_bits;  /*< Stencil bits */
    uint32_t flags;
};

/**
 * @brief D3D Format equivalents — matches GTEX_D3DFormatTable[type] (0xD32698)
 */
constexpr uint32_t D3DFMT_A8R8G8B8       = 21;
constexpr uint32_t D3DFMT_A4R4G4B4       = 26;
constexpr uint32_t D3DFMT_DXT1           = 0x31545844; /*< "DXT1" FOURCC */
constexpr uint32_t D3DFMT_DXT3           = 0x33545844; /*< "DXT3" */
constexpr uint32_t D3DFMT_DXT5           = 0x35545844; /*< "DXT5" */

/**
 * @brief Lookup Tables — hardcoded in ffxivboot.exe at global addresses
 */

/* GTEX_D3DFormatTable[34] at 0xD32698: format_type → D3DFORMAT */
extern const uint32_t GTEX_D3DFormatTable[];

/* GTEX_FormatDescriptorTable[34] at 0xD32AD0: format_type → GTexFormatDesc */
extern const GTexFormatDesc GTEX_FormatDescriptorTable[];

/**
 * @brief GTEX Loading / Parsing Functions — mirroring game code logic
 */

/** GTEX_Format_BitsPerPixel (0x423460) */
inline uint32_t GTEX_Format_BitsPerPixel(uint8_t formatType)
{
    if (formatType >= 34)
        return 0;
    return GTEX_FormatDescriptorTable[formatType].bpp;
}

/** GTEX_Format_IsCompressed (0x423520) */
inline bool GTEX_Format_IsCompressed(uint8_t formatType)
{
    if (formatType >= 34)
        return false;
    return (GTEX_FormatDescriptorTable[formatType].flags & 0x100) != 0;
}

/** GTEX_CalcMipSurfaceOffset (0x4309C0) */
uint32_t GTEX_CalcMipSurfaceOffset(uint32_t index);

/** TextureFactory_CreateFromData (0x430890) */
bool GTEX_DetectAndLoad(const uint8_t* data, uint32_t dataSize);

/** GTEX_ParseHeaderAndCreate (0x4303F0) */
const GTexHeader* GTEX_ValidateHeader(const uint8_t* data, uint32_t dataSize);

/** GTEX_UploadPixelData (0x4302E0) */
uint32_t GTEX_ComputeSurfaceOffset(const GTexHeader& header, uint32_t mipLevel,
                                   uint32_t surface, uint32_t surfaceBase, uint32_t surfaceLimit);

/** GTEX_CreateResource (0x6EE740) — standalone GTEX file resource creation */
void GTEX_CreateResource(const uint8_t* fileData, uint32_t fileSize);

/**
 * @brief High-level GTEX class — for file I/O and format conversion
 */
class GTex
{
public:
    GTex() = default;
    explicit GTex(std::filesystem::path filePath);
    GTex(const uint8_t* data, uint32_t size);

    bool LoadFromFile(const std::filesystem::path& filePath);
    void LoadFromMemory(const uint8_t* data, uint32_t size);

    const GTexHeader& GetHeader() const { return m_header; }
    const std::vector<uint8_t>& GetPixelData() const { return m_pixelData; }

    /** @brief Decode pixel data to 32-bit ARGB (handles DXT1/DXT3/DXT5/ARGB4444/ARGB8888) */
    std::vector<uint8_t> DecodeToARGB8888() const;

    /** @brief Get raw pixel data at a specific mip level + surface */
    const uint8_t* GetSurfaceData(uint32_t mipLevel, uint32_t surface) const;

    uint32_t GetSurfaceDataSize(uint32_t mipLevel) const;

private:
    GTexHeader m_header{};
    std::vector<uint8_t> m_pixelData;

    void ParseHeader(const uint8_t* data, uint32_t size);
    bool ValidateHeader() const;
};
