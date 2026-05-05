#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <span>
#include <string_view>
#include <exception>

/**
 * @file    ChunkReader.h
 * @brief   FFXIV Classic chunk-based data reader
 *
 * The Rapture engine uses 8-byte chunk headers (4B tag + 4B length LE) widely
 * as a general container, covering FDT, SSD, SQWT, DAT, GTEX, SEDB, VGRD,
 * MLRD and other formats.
 *
 * @par Chunk Header Layout (8 bytes)
 * | Offset | Size | Endian | Description |
 * |--------|------|--------|-------------|
 * | 0x00   | 4    | —      | Tag (4 ASCII characters, e.g. "VERS", "DDSF") |
 * | 0x04   | 4    | LE*    | Payload size (configurable as big-endian) |
 * | 0x08   | size | —      | Payload data |
 *
 * @par Usage Example
 * @code
 *   ChunkReader r{data};
 *   while (r.Next()) {
 *       if (r.GetTag() == "VERS")
 *           version = r.GetEntry().u32le(0);
 *   }
 *   for (auto& c : r.Chunks()) {
 *       if (c.tag == "DDSF") surfaces.push_back(c.data);
 *   }
 * @endcode
 */
class ChunkReader
{
public:
    /** @brief 4-character tag type (compile-time comparable) */
    struct Tag
    {
        char c[4]{};
        constexpr Tag() = default;
        constexpr Tag(const char (&s)[5]) : c{s[0], s[1], s[2], s[3]} {}
        constexpr Tag(char a, char b, char cc, char d) : c{a, b, cc, d} {}
        constexpr bool operator==(const Tag& o) const { return c[0] == o.c[0] && c[1] == o.c[1] && c[2] == o.c[2] && c[3] == o.c[3]; }
        constexpr bool operator!=(const Tag& o) const { return !(*this == o); }
        constexpr auto Sv() const -> std::string_view { return {c, 4}; }
    };

    /** @brief Single iteration result — lightweight view, does not own data */
    struct Entry
    {
        Tag          tag;       /**< Chunk tag */
        uint32_t     size = 0;  /**< Payload size in bytes */
        const uint8_t* data = nullptr;  /**< Payload pointer (nullptr if size is 0) */
        uint32_t     offset = 0; /**< Byte offset in source data */

        /** @brief Read uint32 at offset in payload (little-endian)
         *  @pre offset + 3 < size */
        auto U32Le(uint32_t at = 0) const -> uint32_t
        {
            return (static_cast<uint32_t>(data[at]) |
                   (static_cast<uint32_t>(data[at+1]) << 8) |
                   (static_cast<uint32_t>(data[at+2]) << 16) |
                   (static_cast<uint32_t>(data[at+3]) << 24));
        }

        /** @brief Read uint32 at offset in payload (big-endian) */
        auto U32Be(uint32_t at = 0) const -> uint32_t
        {
            return (static_cast<uint32_t>(data[at])   << 24 |
                    static_cast<uint32_t>(data[at+1]) << 16 |
                    static_cast<uint32_t>(data[at+2]) << 8  |
                    static_cast<uint32_t>(data[at+3]));
        }

        /** @brief Payload view */
        auto Payload() const -> std::span<const uint8_t> { return {data, size}; }
    };

    /** @param data Buffer (may be null)
     *  @param size Number of bytes
     *  @param bigEndian If true, interpret chunk size field as big-endian; default LE */
    ChunkReader(const uint8_t* data, size_t size, bool bigEndian = false)
        : m_begin{data}
        , m_end{data + size}
        , m_cursor{data}
        , m_bigEndian{bigEndian}
    {
        initCursor_();
    }

    ChunkReader(std::span<const uint8_t> data, bool bigEndian = false)
        : ChunkReader(data.data(), data.size(), bigEndian) {}


    /** @brief Advance to next chunk (cursor stays at end on failure)
     *  @return Whether successfully positioned */
    auto Next() -> bool
    {
        if (!m_cursor || m_cursor + 8 > m_end) { m_cursor = m_end; return false; }
        uint32_t sz = readLe_(m_cursor + 4);
        if (m_bigEndian) sz = swap32_(sz);
        auto np = m_cursor + 8 + sz;
        if (np > m_end) { m_cursor = m_end; return false; }
        m_cursor = np;
        ++m_index;
        return true;
    }


    auto GetTag()    const -> Tag          { return GetEntry().tag; }
    auto GetSize()   const -> uint32_t     { return GetEntry().size; }
    auto GetData()   const -> const uint8_t* { return GetEntry().data; }
    auto GetOffset() const -> uint32_t     { return GetEntry().offset; }
    auto GetIndex()  const -> uint32_t     { return m_index; }

    /** @brief Get the complete view of the current chunk */
    auto GetEntry() const -> Entry
    {
        if (!m_cursor || m_cursor + 8 > m_end) return {};
        uint32_t sz = readLe_(m_cursor + 4);
        if (m_bigEndian) sz = swap32_(sz);
        return Entry(
            Tag(m_cursor[0], m_cursor[1], m_cursor[2], m_cursor[3]),
            sz,
            sz ? m_cursor + 8 : nullptr,
            static_cast<uint32_t>(m_cursor - m_begin)
        );
    }

    /** @brief Check whether the current tag matches */
    auto IsTag(const Tag& t) const -> bool { return GetEntry().tag == t; }
    auto IsTag(const char (&s)[5]) const -> bool { return IsTag(Tag{s}); }


    auto IsAtEnd() const -> bool { return m_cursor >= m_end; }
    auto IsValid()  const -> bool { return m_index > 0 && !IsAtEnd(); }


    /** @brief Consume all consecutive chunks with matching tag, executing a callback for each
     *  @tparam Fn void(const Entry&) callable
     *  @return Number of chunks consumed */
    template<typename Fn>
    auto ConsumeAll(const Tag& t, Fn&& fn) -> uint32_t
    {
        uint32_t n = 0;
        while (!IsAtEnd() && GetEntry().tag == t) {
            fn(GetEntry());
            Next();
            ++n;
        }
        return n;
    }

    /** @brief Consume one chunk that must match the given tag; throws otherwise
     *  @throws std::runtime_error if tag does not match or already at end */
    auto ConsumeOne(const Tag& t) -> Entry
    {
        if (IsAtEnd() || GetEntry().tag != t)
            fail_(t);
        auto e = GetEntry();
        Next();
        return e;
    }

    /** @brief Consume an optional chunk (if tag matches, otherwise stay in place) */
    auto ConsumeOptional(const Tag& t) -> Entry
    {
        if (!IsAtEnd() && GetEntry().tag == t) {
            auto e = GetEntry();
            Next();
            return e;
        }
        return {};
    }


    void Reset()
    {
        m_cursor = m_begin;
        m_index = 0;
    }

    void SetBigEndian(bool v) { m_bigEndian = v; }
    auto IsBigEndian() const -> bool { return m_bigEndian; }


    struct Iterator
    {
        ChunkReader* r;
        Entry current;
        auto operator++() -> Iterator& { if (r->Next()) current = r->GetEntry(); else current = {}; return *this; }
        auto operator*() const -> const Entry& { return current; }
        auto operator!=(std::nullptr_t) const -> bool { return current.data != nullptr || !r->IsAtEnd(); }
    };

    class ChunkRange
    {
        ChunkReader* m_r;
    public:
        explicit ChunkRange(ChunkReader* r) : m_r{r} { m_r->Reset(); }
        auto begin() -> Iterator { return {m_r, m_r->Next() ? m_r->GetEntry() : Entry{}}; }
        auto end()   -> std::nullptr_t { return nullptr; }
    };

    auto Chunks() -> ChunkRange { return ChunkRange{this}; }

private:
    const uint8_t* m_begin;
    const uint8_t* m_end;
    const uint8_t* m_cursor;
    bool           m_bigEndian;
    uint32_t       m_index = 0;

    void initCursor_()
    {
        if (!m_begin || (m_end - m_begin) < 8)
            m_cursor = m_end;
    }

    static auto readLe_(const uint8_t* p) -> uint32_t
    {
        return static_cast<uint32_t>(p[0]) |
              (static_cast<uint32_t>(p[1]) << 8) |
              (static_cast<uint32_t>(p[2]) << 16) |
              (static_cast<uint32_t>(p[3]) << 24);
    }

    static auto swap32_(uint32_t v) -> uint32_t
    {
        return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
               ((v >> 8) & 0xFF00) | ((v >> 24) & 0xFF);
    }

    [[noreturn]] static void fail_(const Tag& expected)
    {
        throw std::runtime_error(
            "ChunkReader: expected tag '" + std::string(expected.c, 4) + "'");
    }
};
