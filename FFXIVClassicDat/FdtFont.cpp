#include "FdtFont.h"
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <algorithm>

bool FdtFont::Load(const FdtFile& fdt)
{
    m_metrics     = fdt.GetMetrics();
    m_ddsSurfaces = fdt.GetDdsSurfaces();
    m_gtexSurfaces = fdt.GetGtexSurfaces();
    m_heightTable  = fdt.GetHeightTable();
    m_lookupTable  = fdt.GetLookupTable();
    m_glyphData    = fdt.GetGlyphData();

    parseGlyphs_();
    return true;
}

bool FdtFont::Load(const uint8_t* data, uint32_t size, bool bigEndian)
{
    FdtFile fdt;
    if (!fdt.Parse(data, size, bigEndian))
        return false;
    return Load(fdt);
}

bool FdtFont::LoadFromFile(const std::filesystem::path& filePath, bool bigEndian)
{
    FdtFile fdt;
    if (!fdt.LoadFromFile(filePath, bigEndian))
        return false;
    return Load(fdt);
}

const FdtGlyph* FdtFont::GetGlyphByIndex(uint32_t index) const
{
    if (index >= m_glyphs.size())
        return nullptr;
    return &m_glyphs[index];
}

uint32_t FdtFont::GetGlyphCount() const
{
    return static_cast<uint32_t>(m_glyphs.size());
}

uint32_t FdtFont::GetGlyphIndexForChar(uint32_t charCode) const
{
    if (m_lookupTable.empty())
    {
        // No lookup table — fall back to direct character code as index
        if (charCode < m_glyphs.size())
            return charCode;
        return UINT32_MAX;
    }

    const uint8_t* p = m_lookupTable.data();
    size_t entrySize = 2; // Each lookup table entry is 2 bytes (uint16 LE)
    size_t tableEntries = m_lookupTable.size() / entrySize;

    if (charCode < tableEntries)
    {
        uint32_t glyphIndex = static_cast<uint32_t>(p[charCode * 2]) |
                              (static_cast<uint32_t>(p[charCode * 2 + 1]) << 8);

        if (glyphIndex < m_glyphs.size())
            return glyphIndex;
    }

    return UINT32_MAX;
}

bool FdtFont::GetGlyphPixelRect(uint32_t index,
                                 uint32_t atlasWidth, uint32_t atlasHeight,
                                 int& outX, int& outY, int& outW, int& outH) const
{
    const auto* g = GetGlyphByIndex(index);
    if (!g)
        return false;

    outX = static_cast<int>(g->u1 * atlasWidth);
    outY = static_cast<int>(g->v1 * atlasHeight);
    outW = static_cast<int>(g->width);
    outH = static_cast<int>(g->height);
    return true;
}

void FdtFont::parseGlyphs_()
{
    m_glyphs.clear();

    const uint32_t count = m_glyphData.size() / GLYPH_ENTRY_SIZE;
    m_glyphs.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t off = i * GLYPH_ENTRY_SIZE;
        if (off + GLYPH_ENTRY_SIZE > m_glyphData.size())
            break;

        FdtGlyph g;
        std::memcpy(&g.u1,      m_glyphData.data() + off +  0, 4);
        std::memcpy(&g.v1,      m_glyphData.data() + off +  4, 4);
        std::memcpy(&g.u2,      m_glyphData.data() + off +  8, 4);
        std::memcpy(&g.v2,      m_glyphData.data() + off + 12, 4);
        std::memcpy(&g.advance, m_glyphData.data() + off + 16, 4);
        std::memcpy(&g.xOffset, m_glyphData.data() + off + 20, 4);
        std::memcpy(&g.yOffset, m_glyphData.data() + off + 24, 4);
        std::memcpy(&g.width,   m_glyphData.data() + off + 28, 4);
        std::memcpy(&g.height,  m_glyphData.data() + off + 32, 4);
        m_glyphs.push_back(g);
    }
}

void FdtFont::serializeGlyphs_()
{
    m_glyphData.resize(m_glyphs.size() * GLYPH_ENTRY_SIZE);
    for (size_t i = 0; i < m_glyphs.size(); ++i)
    {
        uint32_t off = static_cast<uint32_t>(i) * GLYPH_ENTRY_SIZE;
        const auto& g = m_glyphs[i];
        std::memcpy(m_glyphData.data() + off +  0, &g.u1,      4);
        std::memcpy(m_glyphData.data() + off +  4, &g.v1,      4);
        std::memcpy(m_glyphData.data() + off +  8, &g.u2,      4);
        std::memcpy(m_glyphData.data() + off + 12, &g.v2,      4);
        std::memcpy(m_glyphData.data() + off + 16, &g.advance, 4);
        std::memcpy(m_glyphData.data() + off + 20, &g.xOffset, 4);
        std::memcpy(m_glyphData.data() + off + 24, &g.yOffset, 4);
        std::memcpy(m_glyphData.data() + off + 28, &g.width,   4);
        std::memcpy(m_glyphData.data() + off + 32, &g.height,  4);
    }
}

FdtFile FdtFont::ToFdtFile()
{
    serializeGlyphs_();

    FdtFile fdt;
    fdt.Build(m_metrics, m_ddsSurfaces, m_gtexSurfaces,
              m_heightTable, m_lookupTable, m_glyphData);
    return fdt;
}

void FdtFont::SetGlyphData(const std::vector<uint8_t>& data)
{
    m_glyphData = data;
    parseGlyphs_();
}

void FdtFont::SetHeightTable(const std::vector<uint8_t>& data)
{
    m_heightTable = data;
}

void FdtFont::SetLookupTable(const std::vector<uint8_t>& data)
{
    m_lookupTable = data;
}

void FdtFont::SetGlyphEntry(uint32_t index, const FdtGlyph& glyph)
{
    if (index >= m_glyphs.size())
        m_glyphs.resize(index + 1);
    m_glyphs[index] = glyph;
    serializeGlyphs_();
}
