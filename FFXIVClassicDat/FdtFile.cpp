#include "FdtFile.h"
#include "ChunkReader.h"
#include "BinaryData.h"
#include "DataManager.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>


using Tag = ChunkReader::Tag;

inline constexpr Tag TAG_VERS{"VERS"};
inline constexpr Tag TAG_DDSF{"DDSF"};
inline constexpr Tag TAG_GTEX{"GTEX"};
inline constexpr Tag TAG_TEXC{"TEXC"};
inline constexpr Tag TAG_FSIZ{"FSIZ"};
inline constexpr Tag TAG_FSMX{"FSMX"};
inline constexpr Tag TAG_BSLN{"BSLN"};
inline constexpr Tag TAG_HMGN{"HMGN"};
inline constexpr Tag TAG_HTBL{"HTBL"};
inline constexpr Tag TAG_LTBL{"LTBL"};
inline constexpr Tag TAG_GLYP{"GLYP"};

static void addChunk_(std::vector<uint8_t>& buf, const Tag& tag,
                        const uint8_t* payload, uint32_t size)
{
    buf.insert(buf.end(), tag.c, tag.c + 4);
    buf.push_back(static_cast<uint8_t>(size & 0xFF));
    buf.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((size >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((size >> 24) & 0xFF));
    if (payload && size) buf.insert(buf.end(), payload, payload + size);
}

static auto packU32Le_(uint32_t v) -> std::array<uint8_t, 4>
{
    return {
        static_cast<uint8_t>(v & 0xFF),
        static_cast<uint8_t>((v >> 8) & 0xFF),
        static_cast<uint8_t>((v >> 16) & 0xFF),
        static_cast<uint8_t>((v >> 24) & 0xFF)
    };
}

std::string FdtFile::FileIdToPath(uint32_t fileId)
{
    std::ostringstream ss;
    ss << "data/"
       << std::hex << std::uppercase << std::setfill('0')
       << std::setw(2) << ((fileId >> 24) & 0xFF) << '/'
       << std::setw(2) << ((fileId >> 16) & 0xFF) << '/'
       << std::setw(2) << ((fileId >> 8)  & 0xFF) << '/'
       << std::setw(2) << (fileId & 0xFF) << ".DAT";
    return ss.str();
}

bool FdtFile::Parse(const uint8_t* data, uint32_t size, bool bigEndian)
{
    if (!data || size < 8) return false;

    /* Reset state */
    *this = FdtFile{};

    ChunkReader r{data, size, bigEndian};

    /* 1. Optional VERS (only read when first chunk is VERS) */
    if (!r.IsAtEnd() && r.GetEntry().tag == TAG_VERS) {
        auto e = r.GetEntry();
        if (e.size >= 4)
            m_metrics.version = e.U32Le();
        r.Next();
    }

    /* 2. DDSF loop — all consecutive DDSF chunks added to DDS surface list */
    r.ConsumeAll(TAG_DDSF, [&](const auto& e) {
        m_ddsSurfaces.emplace_back(e.data, e.data + e.size);
    });

    /* 3. GTEX loop — all consecutive GTEX chunks added to GTEX surface list */
    r.ConsumeAll(TAG_GTEX, [&](const auto& e) {
        m_gtexSurfaces.emplace_back(e.data, e.data + e.size);
    });

    /* 4. Parse mandatory chunks, version-dependent */
    if (m_metrics.version >= 4)
        m_metrics.textureCount = r.ConsumeOne(TAG_TEXC).U32Le();

    m_metrics.fontSize = r.ConsumeOne(TAG_FSIZ).U32Le();

    if (m_metrics.version >= 3)
        m_metrics.fontSizeMax = r.ConsumeOne(TAG_FSMX).U32Le();
    else
        m_metrics.fontSizeMax = m_metrics.fontSize;

    m_metrics.baseline = r.ConsumeOne(TAG_BSLN).U32Le();

    if (m_metrics.version >= 2)
        m_metrics.hangMargin = r.ConsumeOne(TAG_HMGN).U32Le();

    /* 5. Data blocks (capture raw bytes) */
    auto e_htbl = r.ConsumeOne(TAG_HTBL);
    m_heightTable.assign(e_htbl.data, e_htbl.data + e_htbl.size);

    auto e_ltbl = r.ConsumeOne(TAG_LTBL);
    m_lookupTable.assign(e_ltbl.data, e_ltbl.data + e_ltbl.size);

    auto e_glyp = r.ConsumeOne(TAG_GLYP);
    m_glyphData.assign(e_glyp.data, e_glyp.data + e_glyp.size);

    return true;
}

bool FdtFile::LoadFromFile(const std::filesystem::path& filePath, bool bigEndian)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return false;

    auto fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    return Parse(buffer.data(), static_cast<uint32_t>(buffer.size()), bigEndian);
}

bool FdtFile::LoadData(uint32_t fileId, bool bigEndian)
{
	BinaryData data = DataManager::GetInstance().LoadData(fileId);
	if (!data.GetLength()) return false;
	return Parse(static_cast<const uint8_t*>(data.GetData()), static_cast<uint32_t>(data.GetLength()), bigEndian);
}

void FdtFile::Build(const Metrics& metrics,
                    const std::vector<std::vector<uint8_t>>& ddsSurfaces,
                    const std::vector<std::vector<uint8_t>>& gtexSurfaces,
                    const std::vector<uint8_t>& heightTable,
                    const std::vector<uint8_t>& lookupTable,
                    const std::vector<uint8_t>& glyphData)
{
    m_metrics      = metrics;
    m_ddsSurfaces  = ddsSurfaces;
    m_gtexSurfaces = gtexSurfaces;
    m_heightTable  = heightTable;
    m_lookupTable  = lookupTable;
    m_glyphData    = glyphData;
}

std::vector<uint8_t> FdtFile::ToBinary() const
{
    std::vector<uint8_t> buf;

    auto u32 = packU32Le_;

    addChunk_(buf, TAG_VERS, u32(m_metrics.version).data(), 4);

    for (auto& s : m_ddsSurfaces)
        addChunk_(buf, TAG_DDSF, s.data(), static_cast<uint32_t>(s.size()));

    for (auto& s : m_gtexSurfaces)
        addChunk_(buf, TAG_GTEX, s.data(), static_cast<uint32_t>(s.size()));

    if (m_metrics.version >= 4)
        addChunk_(buf, TAG_TEXC, u32(m_metrics.textureCount).data(), 4);

    addChunk_(buf, TAG_FSIZ, u32(m_metrics.fontSize).data(), 4);

    if (m_metrics.version >= 3)
        addChunk_(buf, TAG_FSMX, u32(m_metrics.fontSizeMax).data(), 4);

    addChunk_(buf, TAG_BSLN, u32(m_metrics.baseline).data(), 4);

    if (m_metrics.version >= 2)
        addChunk_(buf, TAG_HMGN, u32(m_metrics.hangMargin).data(), 4);

    addChunk_(buf, TAG_HTBL, m_heightTable.data(), static_cast<uint32_t>(m_heightTable.size()));
    addChunk_(buf, TAG_LTBL, m_lookupTable.data(), static_cast<uint32_t>(m_lookupTable.size()));
    addChunk_(buf, TAG_GLYP, m_glyphData.data(), static_cast<uint32_t>(m_glyphData.size()));

    return buf;
}

bool FdtFile::HasVersionField(uint32_t field) const
{
    return m_metrics.version >= field;
}
