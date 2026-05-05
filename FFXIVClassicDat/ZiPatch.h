/**
 * @file ZiPatch.h
 * @brief FFXIV Classic ZiPatch 文件操作库 — 解析、创建、应用 .patch 文件
 *
 * ZiPatch (.patch) 文件是 FINAL FANTASY XIV 1.0 使用的压缩补丁格式。
 * ffxivboot.exe（Patch_ReadFHDRBlock @ 0x7831B0,
 * Patch_ProcessFile @ 0x784700, Patch_ReadBlockHeader @ 0x782ED0）。
 *
 * 文件结构：
 *   - 文件头：12 字节 Magic (0x91 + "ZIPATCH" + \r\n\x1A\n)
 *   - 数据块：size[4, BE] + type[4, ASCII] + data[size] + CRC32[4, BE]
 *
 * 块类型：FHDR, APLY, APFS, ETRY, ADIR, DELD
 *
 * ETRY 子块 (chunk_t)：
 *   mode[4, LE] + prevHash[20] + nextHash[20] + compression[4, LE]
 *   + compressedSize[4, BE] + prevSize[4, BE] + nextSize[4, BE] + data[]
 *
 * 压缩：'N' = 无压缩，'Z' = zlib/deflate
 *
 * @date 2026-5-5
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#define ZIPATCH_HAS_ZLIB

#ifdef ZIPATCH_HAS_ZLIB
#include <zlib.h>
#endif

/** ===========================================================================
 *  常量定义
 *  ============================================================================ */

/** @brief ZiPatch 文件头 Magic（12 字节）
 *  对应 ffxivboot.exe .rdata:0xE0953C */
inline constexpr uint8_t ZIPATCH_MAGIC[12] = {
    0x91, 'Z', 'I', 'P', 'A', 'T', 'C', 'H', 0x0D, 0x0A, 0x1A, 0x0A
};

/** @brief 块类型标签（4 字节 ASCII，字符串表 @ 0x1000014） */
inline constexpr uint32_t BLOCK_FHDR = 0x52444846u;  // "FHDR" (LE)
inline constexpr uint32_t BLOCK_APLY = 0x594C5041u;  // "APLY"
inline constexpr uint32_t BLOCK_APFS = 0x53465041u;  // "APFS"
inline constexpr uint32_t BLOCK_ETRY = 0x59525445u;  // "ETRY"
inline constexpr uint32_t BLOCK_ADIR = 0x52494441u;  // "ADIR"
inline constexpr uint32_t BLOCK_DELD = 0x444C4544u;  // "DELD"

/** @brief ETRY 子块操作模式（单字节字符，LE uint32） */
inline constexpr uint32_t CHUNK_MODE_ADD    = 0x00000041u;  // 'A'
inline constexpr uint32_t CHUNK_MODE_DELETE = 0x00000044u;  // 'D'
inline constexpr uint32_t CHUNK_MODE_MODIFY = 0x0000004Du;  // 'M'

/** @brief 压缩模式（单字节字符，LE uint32） */
inline constexpr uint32_t COMPRESS_NONE = 0x0000004Eu;  // 'N'
inline constexpr uint32_t COMPRESS_ZLIB = 0x0000005Au;  // 'Z'

/** @brief SHA-1 哈希长度 */
inline constexpr size_t SHA1_HASH_SIZE = 20;

/** @brief ETRY 子块头大小 */
inline constexpr size_t CHUNK_HEADER_SIZE = 60;  // 0x3C

/** ===========================================================================
 *  工具函数
 *  ============================================================================ */

/**
 * @brief 从原始字节读取大端 uint32_t
 * @param p_p 字节指针
 * @return 主机字节序 uint32_t
 */
inline uint32_t readU32Be_(const uint8_t* p_p) noexcept {
    return (static_cast<uint32_t>(p_p[0]) << 24) |
           (static_cast<uint32_t>(p_p[1]) << 16) |
           (static_cast<uint32_t>(p_p[2]) <<  8) |
           (static_cast<uint32_t>(p_p[3]));
}

/**
 * @brief 从原始字节读取小端 uint32_t
 * @param p_p 字节指针
 * @return 主机字节序 uint32_t
 */
inline uint32_t readU32Le_(const uint8_t* p_p) noexcept {
    return (static_cast<uint32_t>(p_p[0])) |
           (static_cast<uint32_t>(p_p[1]) <<  8) |
           (static_cast<uint32_t>(p_p[2]) << 16) |
           (static_cast<uint32_t>(p_p[3]) << 24);
}

/**
 * @brief 将大端 uint32_t 写入字节数组
 * @param p_p 目标字节指针（至少 4 字节）
 * @param p_v 要写入的值
 */
inline void writeU32Be_(uint8_t* p_p, uint32_t p_v) noexcept {
    p_p[0] = static_cast<uint8_t>(p_v >> 24);
    p_p[1] = static_cast<uint8_t>(p_v >> 16);
    p_p[2] = static_cast<uint8_t>(p_v >>  8);
    p_p[3] = static_cast<uint8_t>(p_v);
}

/**
 * @brief 将小端 uint32_t 写入字节数组
 * @param p_p 目标字节指针（至少 4 字节）
 * @param p_v 要写入的值
 */
inline void writeU32Le_(uint8_t* p_p, uint32_t p_v) noexcept {
    p_p[0] = static_cast<uint8_t>(p_v);
    p_p[1] = static_cast<uint8_t>(p_v >>  8);
    p_p[2] = static_cast<uint8_t>(p_v >> 16);
    p_p[3] = static_cast<uint8_t>(p_v >> 24);
}

/**
 * @brief 将 uint32_t 块类型标签转换为 4 字节字符串
 * @param p_tag LE 编码标签（例如 BLOCK_FHDR）
 * @return 4 个字符的标签
 */
inline std::string BlockTypeToString(uint32_t p_tag) {
    char buf[5] = {};
    buf[0] = static_cast<char>(p_tag & 0xFF);
    buf[1] = static_cast<char>((p_tag >> 8) & 0xFF);
    buf[2] = static_cast<char>((p_tag >> 16) & 0xFF);
    buf[3] = static_cast<char>((p_tag >> 24) & 0xFF);
    return {buf, 4};
}

/** ===========================================================================
 *  CRC32 (RFC 1952 / zlib)
 *  ============================================================================ */

/**
 * @brief CRC32 查找表（RFC 1952 多项式）
 */
inline const std::array<uint32_t, 256>& Crc32Table() {
    static const auto tbl = []() {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    return tbl;
}

/**
 * @brief 计算 CRC32（与 zlib.crc32 兼容）
 * @param p_data 输入数据
 * @param p_prev 之前的 CRC 值（用于继续计算）
 * @return CRC32 校验和值
 */
inline uint32_t Crc32Compute(std::span<const uint8_t> p_data, uint32_t p_prev = 0) {
    uint32_t c = p_prev ^ 0xFFFFFFFFu;
    for (auto b : p_data) {
        c = Crc32Table()[(c ^ b) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

/** ===========================================================================
 *  SHA-1（文件哈希验证）
 *  ============================================================================ */

/**
 * @brief SHA-1 哈希计算器
 *
 * 实现 FIPS 180-4 SHA-1 算法。
 * 用于验证 ETRY 块中的 prev_hash / next_hash。
 */
class Sha1 {
public:
    Sha1() { Reset(); }

    void Reset() {
        m_h[0] = 0x67452301u; m_h[1] = 0xEFCDAB89u;
        m_h[2] = 0x98BADCFEu; m_h[3] = 0x10325476u;
        m_h[4] = 0xC3D2E1F0u;
        m_len = 0;
        m_buf.fill(0);
        m_buf_pos = 0;
    }

    void Update(std::span<const uint8_t> p_data) {
        for (auto b : p_data) {
            m_buf[m_buf_pos++] = b;
            m_len += 8;
            if (m_buf_pos == 64) {
                ProcessBlock_();
                m_buf_pos = 0;
            }
        }
    }

    void Update(const void* p_data, size_t p_size) {
        Update({static_cast<const uint8_t*>(p_data), p_size});
    }

    /**
     * @brief 完成 SHA-1 计算
     * @return 20 字节哈希值
     */
    std::array<uint8_t, 20> Finalize() {
        uint64_t total_bits = m_len;
        m_buf[m_buf_pos++] = 0x80;
        if (m_buf_pos > 56) {
            while (m_buf_pos < 64) m_buf[m_buf_pos++] = 0;
            ProcessBlock_();
            m_buf_pos = 0;
        }
        while (m_buf_pos < 56) m_buf[m_buf_pos++] = 0;
        for (int i = 7; i >= 0; --i) {
            m_buf[56 + i] = static_cast<uint8_t>(total_bits >> ((7 - i) * 8));
        }
        m_buf_pos = 64;
        ProcessBlock_();

        std::array<uint8_t, 20> result;
        for (int i = 0; i < 5; ++i) {
            writeU32Be_(&result[i * 4], m_h[i]);
        }
        return result;
    }

private:
    void ProcessBlock_() {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = readU32Be_(&m_buf[i * 4]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rotl_(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }
        uint32_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3], e = m_h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & d); k = 0x5A827999u; }
            else if (i < 40) { f = b ^ c ^ d;           k = 0x6ED9EBA1u; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
            else             { f = b ^ c ^ d;           k = 0xCA62C1D6u; }
            uint32_t temp = rotl_(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rotl_(b, 30); b = a; a = temp;
        }
        m_h[0] += a; m_h[1] += b; m_h[2] += c; m_h[3] += d; m_h[4] += e;
    }

    static uint32_t rotl_(uint32_t v, int n) noexcept {
        return (v << n) | (v >> (32 - n));
    }

    uint32_t m_h[5];
    uint64_t m_len;
    std::array<uint8_t, 64> m_buf;
    size_t m_buf_pos;
};

/** ===========================================================================
 *  ZiPatchChunk — ETRY 子块
 *  ============================================================================ */

/**
 * @brief ETRY 块中的文件操作子块 (chunk_t)
 *
 * 每个块代表对单个文件的一个操作（添加/删除/修改）。
 * 结构与 ffxivboot.exe 中 Patch_SwapChunkSizes (0x782EA0) 处理的格式匹配。
 */
struct ZiPatchChunk {
    /** @brief 操作模式：'A'=添加, 'D'=删除, 'M'=修改 */
    uint32_t mode = 0;

    /** @brief 操作前的 SHA-1 哈希（20 字节，如果没有旧文件则全为零） */
    std::array<uint8_t, SHA1_HASH_SIZE> prev_hash{};

    /** @brief 操作后的 SHA-1 哈希（20 字节，删除模式下全为零） */
    std::array<uint8_t, SHA1_HASH_SIZE> next_hash{};

    /** @brief 压缩模式：'N'=无压缩, 'Z'=zlib */
    uint32_t compression = COMPRESS_NONE;

    /** @brief 压缩后的数据大小（补丁文件中的字节数） */
    uint32_t compressed_size = 0;

    /** @brief 操作前文件的未压缩大小 */
    uint32_t prev_size = 0;

    /** @brief 操作后文件的未压缩大小 */
    uint32_t next_size = 0;

    /** @brief 原始压缩数据 */
    std::vector<uint8_t> data;

    /** ========================================================================
     *  Serialization
     *  ======================================================================== */

    /**
     * @brief Parse chunk header from raw bytes
     * @param p Pointer to start of chunk data
     * @return Actual bytes read (header + data)
     */
    size_t ParseFrom(const uint8_t* p) {
        mode        = readU32Le_(p);      p += 4;
        std::memcpy(prev_hash.data(), p, SHA1_HASH_SIZE); p += SHA1_HASH_SIZE;
        std::memcpy(next_hash.data(), p, SHA1_HASH_SIZE); p += SHA1_HASH_SIZE;
        compression = readU32Le_(p);      p += 4;
        compressed_size = readU32Be_(p);  p += 4;
        prev_size       = readU32Be_(p);  p += 4;
        next_size       = readU32Be_(p);  p += 4;
        data.assign(p, p + compressed_size);
        return CHUNK_HEADER_SIZE + compressed_size;
    }

    /**
     * @brief Serialize chunk to binary (header + data)
     * @param out Output buffer
     */
    void WriteTo(std::vector<uint8_t>& out) const {
        size_t off = out.size();
        out.resize(off + CHUNK_HEADER_SIZE + data.size());
        auto* p = out.data() + off;
        writeU32Le_(p, mode);             p += 4;
        std::memcpy(p, prev_hash.data(), SHA1_HASH_SIZE); p += SHA1_HASH_SIZE;
        std::memcpy(p, next_hash.data(), SHA1_HASH_SIZE); p += SHA1_HASH_SIZE;
        writeU32Le_(p, compression);      p += 4;
        writeU32Be_(p, compressed_size);  p += 4;
        writeU32Be_(p, prev_size);        p += 4;
        writeU32Be_(p, next_size);        p += 4;
        if (!data.empty()) {
            std::memcpy(p, data.data(), data.size());
        }
    }

    /** ========================================================================
     *  Helper methods
     *  ======================================================================== */

    /** @brief Operation mode name */
    std::string ModeName() const {
        switch (mode) {
            case CHUNK_MODE_ADD:    return "add";
            case CHUNK_MODE_DELETE: return "delete";
            case CHUNK_MODE_MODIFY: return "modify";
            default:                return "unknown";
        }
    }

    /** @brief Compression mode name */
    std::string CompressionName() const {
        switch (compression) {
            case COMPRESS_NONE: return "none";
            case COMPRESS_ZLIB: return "zlib";
            default:            return "unknown";
        }
    }

    /**
     * @brief Decompress chunk data
     * @return Decompressed data (requires ZIPATCH_HAS_ZLIB for zlib support)
     * @throws std::runtime_error if zlib not enabled or decompression fails
     */
    std::vector<uint8_t> Decompress() const;

    /**
     * @brief Compute SHA-1 hash of decompressed data
     * @return 20-byte SHA-1 hash
     */
    std::array<uint8_t, 20> ComputeNextHash() const {
        auto decoded = Decompress();
        Sha1 sha1;
        sha1.Update(decoded);
        return sha1.Finalize();
    }
};

/** ===========================================================================
 *  ZiPatchBlock — Data block
 *  ============================================================================ */

/**
 * @brief A data block in a ZiPatch file
 *
 * Each block corresponds to one operation: file header info (FHDR),
 * apply options (APLY), file system total (APFS), file entry (ETRY),
 * create directory (ADIR), delete directory (DELD).
 *
 * Block structure (verified from binary):
 *   size[4, BE] + type[4, ASCII] + data[size] + CRC32[4, BE]
 *   CRC32 covers type + data, not the size field.
 */
struct ZiPatchBlock {
    /** @brief Block type (e.g. BLOCK_FHDR) */
    uint32_t type = 0;

    /** @brief Data size (bytes) */
    uint32_t size = 0;

    /** @brief Raw data */
    std::vector<uint8_t> data;

    /** @brief CRC32 checksum value */
    uint32_t crc = 0;

    /** @brief Whether CRC check passed (has_value means CRC was read) */
    std::optional<bool> crc_ok;

    /** @brief Block offset within file */
    size_t file_offset = 0;

    /** ========================================================================
     *  Type checks
     *  ======================================================================== */

    bool IsFhdr() const { return type == BLOCK_FHDR; }
    bool IsAply() const { return type == BLOCK_APLY; }
    bool IsApfs() const { return type == BLOCK_APFS; }
    bool IsEtry() const { return type == BLOCK_ETRY; }
    bool IsAdir() const { return type == BLOCK_ADIR; }
    bool IsDeld() const { return type == BLOCK_DELD; }

    /** ========================================================================
     *  FHDR fields (valid for FHDR blocks only)
     *  ======================================================================== */

    /**
     * @brief FHDR version number (observed 0x0200 = FileHeaderV2)
     */
    uint32_t FhdrVersion() const {
        return readU32Be_(data.data());
    }

    /**
     * @brief FHDR result code ("DIFF" or "HIST")
     */
    std::string FhdrResult() const {
        return std::string(reinterpret_cast<const char*>(data.data() + 4),
                           reinterpret_cast<const char*>(data.data() + 8));
    }

    /** @brief Entry file count (number of ETRY blocks) */
    uint32_t FhdrEntryCount() const {
        return readU32Be_(data.data() + 8);
    }

    /** @brief Add directory count (number of ADIR blocks) */
    uint32_t FhdrAdddirCount() const {
        return readU32Be_(data.data() + 12);
    }

    /** @brief Delete directory count (number of DELD blocks) */
    uint32_t FhdrDeldirCount() const {
        return readU32Be_(data.data() + 16);
    }

    /** ========================================================================
     *  APLY fields
     *  ======================================================================== */

    /**
     * @brief Get the three uint32_t values from an APLY block
     * @return (option type, reserved field, boolean value)
     */
    std::tuple<uint32_t, uint32_t, uint32_t> ApplyValues() const {
        return {
            readU32Be_(data.data()),
            readU32Be_(data.data() + 4),
            readU32Be_(data.data() + 8)
        };
    }

    /** ========================================================================
     *  ADIR / DELD / ETRY path
     *  ======================================================================== */

    /**
     * @brief Get path string from block (ADIR, DELD, ETRY)
     *
     * Paths use Shift-JIS encoding (FFXIV Classic is Japan-only).
     * @return Decoded path string
     */
    std::string Path() const;

    /** ========================================================================
     *  ETRY sub-blocks
     *  ======================================================================== */

    /**
     * @brief Parse all sub-blocks in an ETRY block
     * @return List of sub-blocks
     */
    std::vector<ZiPatchChunk> Chunks() const;

    /** ========================================================================
     *  Serialization
     *  ======================================================================== */

    /**
     * @brief Serialize block to full binary (size + type + data + CRC)
     * @param out Output buffer
     * @param include_crc Whether to include CRC32 trailer
     */
    void WriteTo(std::vector<uint8_t>& out, bool include_crc = true) const {
        size_t off = out.size();
        out.resize(off + 8 + data.size() + (include_crc ? 4 : 0));
        auto* p = out.data() + off;
        writeU32Be_(p, size);             p += 4;
        writeU32Le_(p, type);             p += 4;
        if (!data.empty()) {
            std::memcpy(p, data.data(), data.size());
            p += data.size();
        }
        if (include_crc) {
            writeU32Be_(p, crc);
        }
    }
};

/** ===========================================================================
 *  ZiPatchFile — Patch file reader
 *  ============================================================================ */

/**
 * @brief ZiPatch file reader
 *
 * Parses .patch files, providing ability to iterate all blocks and sub-blocks.
 * All CRC32 checksums are verified; failed blocks can be checked via the
 * crc_ok field.
 *
 * @code
 * ZiPatchFile pf("D2010.09.19.0000.patch");
 * std::cout << pf.Summary() << std::endl;
 * for (auto& block : pf.Blocks()) {
 *     if (block.IsEtry()) {
 *         for (auto& chunk : block.Chunks()) {
 *             auto data = chunk.Decompress();
 *         }
 *     }
 * }
 * @endcode
 */
class ZiPatchFile {
public:
    /**
     * @brief Construct and parse from file path
     * @param filepath .patch file path
     * @throws std::runtime_error if file not found or invalid format
     */
    explicit ZiPatchFile(const std::filesystem::path& filepath);

    /**
     * @brief Parse from memory buffer
     * @param buffer Buffer containing complete .patch file contents
     * @throws std::runtime_error if invalid format
     */
    explicit ZiPatchFile(std::span<const uint8_t> buffer);

    /** ── Basic information ────────────────────────────────────────────────── */

    /** @brief File path (empty when parsed from memory) */
    const std::filesystem::path& Filepath() const { return m_filepath; }

    /** @brief All parsed blocks */
    const std::vector<ZiPatchBlock>& Blocks() const { return m_blocks; }

    /** @brief FHDR version number */
    uint32_t Version() const { return m_version; }

    /** @brief FHDR result code */
    const std::string& Result() const { return m_result; }

    /** @brief Declared file entry count */
    uint32_t DeclaredEntries() const { return m_declared_entries; }

    /** @brief Declared add directory count */
    uint32_t DeclaredAdddir() const { return m_declared_adddir; }

    /** @brief Declared delete directory count */
    uint32_t DeclaredDeldir() const { return m_declared_deldir; }

    /** @brief Total block count */
    size_t BlockCount() const { return m_blocks.size(); }

    /** ── Queries ──────────────────────────────────────────────────────────── */

    /** @brief Get list of blocks of a given type */
    std::vector<const ZiPatchBlock*> BlocksOf(uint32_t type) const;

    /** @brief Check for CRC failures */
    bool HasCrcFailures() const { return !m_crc_failures.empty(); }

    /** @brief List of block indices with CRC failures */
    const std::vector<size_t>& CrcFailures() const { return m_crc_failures; }

    /** ── Summary ──────────────────────────────────────────────────────────── */

    /**
     * @brief Generate file content summary
     * @return Multi-line summary string
     */
    std::string Summary() const;

private:
    void parse_(std::span<const uint8_t> buffer);

    std::filesystem::path m_filepath;
    std::vector<ZiPatchBlock> m_blocks;
    std::vector<size_t> m_crc_failures;

    uint32_t m_version = 0;
    std::string m_result;
    uint32_t m_declared_entries = 0;
    uint32_t m_declared_adddir = 0;
    uint32_t m_declared_deldir = 0;
};

/** ===========================================================================
 *  ZiPatchBuilder — Patch file builder
 *  ============================================================================ */

/**
 * @brief ZiPatch patch file builder
 *
 * For programmatic creation of .patch files, usable for patch servers or tools.
 * Generated files include correct size prefix and CRC32 trailer.
 *
 * @code
 * ZiPatchBuilder builder;
 * builder.SetFhdr(0x0200, "HIST", 1, 1, 0);
 * builder.AddAdir("data/test");
 * builder.AddEtry("data/test/file.bin", {chunk});
 * builder.Write("output.patch");
 * @endcode
 */
class ZiPatchBuilder {
public:
    ZiPatchBuilder() = default;

    /**
     * @brief Set FHDR block
     * @param version Version number (default 0x0200)
     * @param result Result code (default "HIST")
     * @param num_entry ETRY entry count
     * @param num_adddir ADIR directory count
     * @param num_deldir DELD directory count
     */
    void SetFhdr(uint32_t version, const std::string& result,
                 uint32_t num_entry, uint32_t num_adddir, uint32_t num_deldir);

    /**
     * @brief Add APFS block (file system total)
     * @param total_file_size Incremental total file size
     * @param total_disk_size Incremental total disk size
     */
    void AddApfs(uint64_t total_file_size, uint64_t total_disk_size);

    /**
     * @brief Add APLY option block
     * @param option_type Option type (1 or 2)
     * @param enabled Whether enabled
     */
    void AddAply(uint32_t option_type, bool enabled);

    /**
     * @brief Add ADIR block (create directory)
     * @param path Relative path (ASCII)
     */
    void AddAdir(const std::string& path);

    /**
     * @brief Add DELD block (delete directory)
     * @param path Relative path (ASCII)
     */
    void AddDeld(const std::string& path);

    /**
     * @brief Add ETRY block (file entry)
     * @param path Relative path (ASCII)
     * @param chunks List of file operation sub-blocks
     */
    void AddEtry(const std::string& path, const std::vector<ZiPatchChunk>& chunks);

    /**
     * @brief Write patch to file
     * @param filepath Output file path
     */
    void Write(const std::filesystem::path& filepath) const;

    /**
     * @brief Serialize patch to byte array
     * @return Complete .patch file contents
     */
    std::vector<uint8_t> ToBytes() const;

private:
    // void appendBlock_(std::vector<uint8_t>& out, const ZiPatchBlock& block) const;

    std::optional<ZiPatchBlock> m_fhdr;
    std::vector<ZiPatchBlock> m_blocks;
};

/** ===========================================================================
 *  ZiPatchApplier — Patch applier
 *  ============================================================================ */

/**
 * @brief Patch apply result
 */
struct ApplyResult {
    /** @brief Files added count */
    size_t files_added = 0;

    /** @brief Files modified count */
    size_t files_modified = 0;

    /** @brief Files deleted count */
    size_t files_deleted = 0;

    /** @brief Directories created count */
    size_t dirs_created = 0;

    /** @brief Directories deleted count */
    size_t dirs_deleted = 0;

    /** @brief Bytes written count */
    uint64_t bytes_written = 0;

    /** @brief Skipped operations count */
    size_t skipped = 0;

    /** @brief Error message list */
    std::vector<std::string> errors;

    /** @brief Whether all succeeded */
    bool Ok() const { return errors.empty(); }

    /** @brief Total file operations count */
    size_t TotalFiles() const { return files_added + files_modified + files_deleted; }

    /**
     * @brief Generate result summary
     */
    std::string Summary() const;
};

/**
 * @brief ZiPatch patch applier
 *
 * Apply parsed patch file to a specified game install directory.
 * Implements the functionality corresponding to Patch_ProcessETRY (0x783B50),
 * Patch_ProcessADIR (0x783EA0), Patch_ProcessDELD (0x783FF0) in ffxivboot.exe:
 *   - ADIR: create directory
 *   - DELD: delete empty directory
 *   - ETRY: execute file add/modify/delete by chunk mode
 *   - SHA-1 verification: verify existing file hash before modify/delete
 *
 * @code
 * ZiPatchFile pf("patch.patch");
 * ZiPatchApplier applier(pf);
 * auto result = applier.Apply("D:\\Games\\FFXIV", false);
 * if (!result.Ok()) { ... }
 * @endcode
 */
class ZiPatchApplier {
public:
    /**
     * @brief Constructor
     * @param patch Parsed patch file
     */
    explicit ZiPatchApplier(const ZiPatchFile& patch) : m_patch(patch) {}

    /**
     * @brief Apply patch to target directory
     *
     * @param install_dir Game install root directory
     * @param dry_run If true, preview only without modifying filesystem
     * @param no_verify If true, skip SHA-1 hash verification
     * @param backup_dir If non-empty, backup files to this directory before modification
     * @param progress Progress callback (args: current block index, total blocks)
     * @return Apply result
     */
    ApplyResult Apply(
        const std::filesystem::path& install_dir,
        bool dry_run = false,
        bool no_verify = false,
        const std::filesystem::path& backup_dir = {},
        std::function<void(size_t, size_t)> progress = {}
    );

private:
    void backupIfNeeded_(const std::filesystem::path& filepath,
                          const std::filesystem::path& backup_dir);

    const ZiPatchFile& m_patch;
};
