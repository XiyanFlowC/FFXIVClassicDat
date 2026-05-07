#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>
#include "FdtFile.h"

/**
 * @struct FdtGlyph
 * @brief 单个字形条目（初步 36 字节结构）。
 *
 * GLYP 块包含每个字形的渲染数据。当前最佳估计为 9 × float32 LE = 每个条目 36 字节：
 *   [ 0] u1       — UV 左值
 *   [ 4] v1       — UV 上值
 *   [ 8] u2       — UV 右值
 *   [12] v2       — UV 下值
 *   [16] advance  — 水平前进量（像素）
 *   [20] xOffset  — 从画笔位置的 X 偏移
 *   [24] yOffset  — 从画笔位置的 Y 偏移
 *   [28] width    — 字形像素宽度
 *   [32] height   — 字形像素高度
 *
 * 注意：这是初步结果。条目大小和字段布局取决于 FDT 版本和字体编解码器。
 * 实际游戏代码通过高度/查找表的间接访问来读取 GLYP 缓冲区。
 */
struct FdtGlyph
{
    float u1      = 0.0f;
    float v1      = 0.0f;
    float u2      = 0.0f;
    float v2      = 0.0f;
    float advance = 0.0f;
    float xOffset = 0.0f;
    float yOffset = 0.0f;
    float width   = 0.0f;
    float height  = 0.0f;
};

/**
 * @brief FdtFont — 运行时字体数据访问器。
 *
 * 加载 FdtFile 并为字体编辑器或渲染管线提供每个字形查询访问。
 *
 * @par 用法：
 * @code
 *   FdtFile fdt;
 *   fdt.LoadFromFile("path/to/font.DAT");
 *   FdtFont font;
 *   font.Load(fdt);
 *   auto* glyph = font.GetGlyphByIndex(0x41);  // 'A'
 * @endcode
 */
class FdtFont
{
public:
    static constexpr uint32_t GLYPH_ENTRY_SIZE = 36;  /* 初步字形条目大小 */

    FdtFont() = default;

    /** @brief 从已解析的 FdtFile 加载 */
    bool Load(const FdtFile& p_fdt);

    /** @brief 从原始 FDT 二进制数据加载 */
    bool Load(const uint8_t* p_data, uint32_t p_size, bool p_bigEndian = false);

    /** @brief 从文件加载 */
    bool LoadFromFile(const std::filesystem::path& p_filePath, bool p_bigEndian = false);

    /** @brief 获取字体度量 */
    const FdtFile::Metrics& GetMetrics() const { return m_metrics; }

    /**
     * @brief 获取指定字符代码的字形索引（通过查找表）。
     * @param p_charCode 字符代码 (Unicode code point)
     * @return 字形索引，如果字符代码超出范围则返回 UINT32_MAX
     */
    uint32_t GetGlyphIndexForChar(uint32_t p_charCode) const;

    /**
     * @brief 获取直接索引处的字形（基于 0，顺序）。
     * @note 实际游戏使用 heightTable + lookupTable 进行间接访问，
     *       而不是直接索引。这返回 GLYP 缓冲区中的顺序条目。
     */
    const FdtGlyph* GetGlyphByIndex(uint32_t p_index) const;
    uint32_t GetGlyphCount() const;

    /**
     * @brief 计算字形在图集中的像素坐标。
     * @param p_index 字形索引
     * @param p_atlasWidth 图集纹理宽度。
     * @param p_atlasHeight 图集纹理高度。
     * @param p_outX 输出 X 坐标
     * @param p_outY 输出 Y 坐标
     * @param p_outW 输出宽度
     * @param p_outH 输出高度
     * @return 如果索引超出范围，返回 false。
     */
    bool GetGlyphPixelRect(uint32_t p_index,
                           uint32_t p_atlasWidth, uint32_t p_atlasHeight,
                           int& p_outX, int& p_outY, int& p_outW, int& p_outH) const;

    /** @brief 原始数据访问器 */
    const std::vector<uint8_t>& GetGlyphData()   const { return m_glyphData; }
    const std::vector<uint8_t>& GetHeightTable() const { return m_heightTable; }
    const std::vector<uint8_t>& GetLookupTable() const { return m_lookupTable; }

    /** @brief 图集表面访问器 */
    const std::vector<std::vector<uint8_t>>& GetGtexSurfaces() const { return m_gtexSurfaces; }
    const std::vector<std::vector<uint8_t>>& GetDdsSurfaces()  const { return m_ddsSurfaces; }

    /** @brief 构建修改后的字形数据的新 FdtFile */
    FdtFile ToFdtFile();

    /** @brief 编辑器修改帮助函数 */
    void SetGlyphData(const std::vector<uint8_t>& p_data);
    void SetHeightTable(const std::vector<uint8_t>& p_data);
    void SetLookupTable(const std::vector<uint8_t>& p_data);
    void SetGlyphEntry(uint32_t p_index, const FdtGlyph& p_glyph);

private:
    FdtFile::Metrics                  m_metrics;
    std::vector<std::vector<uint8_t>> m_ddsSurfaces;
    std::vector<std::vector<uint8_t>> m_gtexSurfaces;
    std::vector<uint8_t>             m_heightTable;
    std::vector<uint8_t>             m_lookupTable;
    std::vector<uint8_t>             m_glyphData;
    std::vector<FdtGlyph>            m_glyphs;

    void parseGlyphs_();
    void serializeGlyphs_();
};
