#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

/**
 * @file    FdtFile.h
 * @brief   FDT (Font Data Table) binary parser and generator
 *
 * FDT is a chunk-based bitmap font data container, with low-level iteration
 * provided by ChunkReader. Chunk ordering and semantics have been verified
 * by decompiling ffxivboot.exe:
 *   FontData_ParseFDT          @ 0x75A8F0  (Component/Font/FontData.cpp)
 *   RaptureFontData_ParseFDT   @ 0x4C54F0  (Application/Scene/SceneThread.cpp)
 *
 * 11 chunk types: VERS DDSF GTEX TEXC FSIZ FSMX BSLN HMGN HTBL LTBL GLYP
 */

/**
 * @brief 表示 FDT 字体文件的类,用于解析、构建和操作 FDT 二进制数据。
 */
class FdtFile
{
public:
    struct Metrics
    {
        uint32_t version      = 0;  /**< FD +0x10, from VERS chunk */
        uint32_t textureCount = 0;  /**< FD +0x14, from TEXC chunk (version>=4) */
        uint32_t fontSize     = 0;  /**< FD +0x24, from FSIZ chunk */
        uint32_t fontSizeMax  = 0;  /**< FD +0x28, from FSMX chunk (version>=3) */
        uint32_t baseline     = 0;  /**< FD +0x30, from BSLN chunk */
        uint32_t hangMargin   = 0;  /**< FD +0x2C, from HMGN chunk (version>=2) */
    };


    FdtFile() = default;


    /**
     * @brief Parse FDT binary data
     * @param bigEndian If true, interpret chunk size field as big-endian (default LE)
     * @return true on success
     */
    bool Parse(const uint8_t* data, uint32_t size, bool bigEndian = false);


    bool LoadFromFile(const std::filesystem::path& filePath, bool bigEndian = false);

    /**
     * @brief Load FDT data by file ID
     * @param fileId File ID to load
     * @param bigEndian Big-endian mode
     * @return true on success
     */
    bool LoadData(uint32_t fileId, bool bigEndian = false);


    void Build(const Metrics& metrics,
               const std::vector<std::vector<uint8_t>>& ddsSurfaces,
               const std::vector<std::vector<uint8_t>>& gtexSurfaces,
               const std::vector<uint8_t>& heightTable,
               const std::vector<uint8_t>& lookupTable,
               const std::vector<uint8_t>& glyphData);

    auto ToBinary() const -> std::vector<uint8_t>;


    auto GetMetrics()      const -> const Metrics&               { return m_metrics; }
    auto GetDdsSurfaces()  const -> const auto&                  { return m_ddsSurfaces; }
    auto GetGtexSurfaces() const -> const auto&                  { return m_gtexSurfaces; }
    auto GetHeightTable()  const -> const std::vector<uint8_t>&  { return m_heightTable; }
    auto GetLookupTable()  const -> const std::vector<uint8_t>&  { return m_lookupTable; }
    auto GetGlyphData()    const -> const std::vector<uint8_t>&  { return m_glyphData; }

    bool HasVersionField(uint32_t field) const;


    /** @brief Known 5 FDT font file IDs (data/00/0C/00/00.DAT ~ 04.DAT) */
    static inline constexpr uint32_t FONT_FILE_IDS[] = {
        0x000C0000, 0x000C0001, 0x000C0002, 0x000C0003, 0x000C0004
    };
    static constexpr size_t FONT_FILE_COUNT = 5;

    /** @brief Convert file ID to DAT path (e.g. 0x000C0001 → data/00/0C/00/01.DAT) */
    static std::string FileIdToPath(uint32_t fileId);

private:
    Metrics                           m_metrics;
    std::vector<std::vector<uint8_t>> m_ddsSurfaces;
    std::vector<std::vector<uint8_t>> m_gtexSurfaces;
    std::vector<uint8_t>              m_heightTable;
    std::vector<uint8_t>              m_lookupTable;
    std::vector<uint8_t>              m_glyphData;
};
