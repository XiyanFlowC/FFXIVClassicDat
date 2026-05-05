/**
 * @file ZiPatch.cpp
 * @brief ZiPatch library implementation — patch file parsing, building, applying
 */

#include "ZiPatch.h"

#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

#ifdef ZIPATCH_HAS_ZLIB
#include <zlib.h>
#endif


std::vector<uint8_t> ZiPatchChunk::Decompress() const {
    if (compression == COMPRESS_NONE) {
        return data;
    }
    if (compression == COMPRESS_ZLIB) {
#ifdef ZIPATCH_HAS_ZLIB
        std::vector<uint8_t> result(next_size > 0 ? next_size : data.size() * 4);
        z_stream strm = {};
        strm.next_in = const_cast<Bytef*>(data.data());
        strm.avail_in = static_cast<uInt>(data.size());

        if (inflateInit(&strm) != Z_OK) {
            throw std::runtime_error("ZiPatchChunk::Decompress: inflateInit failed");
        }

        strm.next_out = result.data();
        strm.avail_out = static_cast<uInt>(result.size());

        int ret = inflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END) {
            /* If buffer is too small, try expanding */
            if (ret == Z_OK || ret == Z_BUF_ERROR) {
                size_t used = strm.total_out;
                result.resize(result.size() * 2);
                strm.next_out = result.data() + used;
                strm.avail_out = static_cast<uInt>(result.size() - used);
                ret = inflate(&strm, Z_FINISH);
            }
        }

        inflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            throw std::runtime_error("ZiPatchChunk::Decompress: inflate failed");
        }

        result.resize(strm.total_out);
        return result;
#else
        throw std::runtime_error(
            "ZiPatchChunk::Decompress: zlib compression requires ZIPATCH_HAS_ZLIB. "
            "Define ZIPATCH_HAS_ZLIB and link zlib to enable decompression.");
#endif
    }
    throw std::runtime_error("ZiPatchChunk::Decompress: unknown compression mode");
}

std::string ZiPatchBlock::Path() const {
    if (!IsAdir() && !IsDeld() && !IsEtry()) {
        throw std::runtime_error("ZiPatchBlock::Path: block type has no path");
    }
    if (data.size() < 4) return {};
    uint32_t path_len = readU32Be_(data.data());
    if (4 + path_len > data.size()) {
        path_len = static_cast<uint32_t>(data.size() - 4);
    }
    const auto* p = data.data() + 4;
    /* FFXIV Classic paths use Shift-JIS encoding.
     *  Falls back to ASCII on non-Japanese environments. */
    std::string result;
    result.reserve(path_len);
    for (uint32_t i = 0; i < path_len; ) {
        uint8_t c = p[i];
        if (c < 0x80) {
            /* ASCII / control characters */
            result.push_back(static_cast<char>(c));
            ++i;
        } else if (c >= 0xA1 && c <= 0xDF) {
            /* JIS X 0201 half-width katakana */
            result.push_back(static_cast<char>(c));
            ++i;
        } else if (c >= 0x81 && c <= 0x9F || c >= 0xE0 && c <= 0xFC) {
            /* Shift-JIS double-byte characters */
            if (i + 1 < path_len) {
                result.push_back(static_cast<char>(c));
                result.push_back(static_cast<char>(p[i + 1]));
                i += 2;
            } else {
                result.push_back('?');
                ++i;
            }
        } else {
            result.push_back('?');
            ++i;
        }
    }
    return result;
}


std::vector<ZiPatchChunk> ZiPatchBlock::Chunks() const {
    if (!IsEtry()) {
        throw std::runtime_error("ZiPatchBlock::Chunks: not an ETRY block");
    }
    if (data.size() < 4) return {};
    uint32_t path_len = readU32Be_(data.data());
    size_t pos = 4 + path_len;
    if (pos + 4 > data.size()) return {};
    uint32_t count = readU32Be_(data.data() + pos);
    pos += 4;

    std::vector<ZiPatchChunk> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (pos + CHUNK_HEADER_SIZE > data.size()) break;
        ZiPatchChunk chunk;
        size_t consumed = chunk.ParseFrom(data.data() + pos);
        pos += consumed;
        result.push_back(std::move(chunk));
    }
    return result;
}


ZiPatchFile::ZiPatchFile(const std::filesystem::path& filepath)
    : m_filepath(filepath)
{
    /* Read entire file into memory */
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath.string());
    }
    auto file_size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));
    if (file.fail()) {
        throw std::runtime_error("Failed to read file: " + filepath.string());
    }
    parse_(buffer);
}

ZiPatchFile::ZiPatchFile(std::span<const uint8_t> buffer) {
    parse_(buffer);
}

void ZiPatchFile::parse_(std::span<const uint8_t> buffer) {
    /* Verify file header */
    if (buffer.size() < 12 ||
        std::memcmp(buffer.data(), ZIPATCH_MAGIC, 12) != 0) {
        throw std::runtime_error("Invalid ZiPatch file: bad magic bytes");
    }

    size_t offset = 12;

    while (offset + 8 <= buffer.size()) {
        /* Read block: size[4, BE] + type[4, ASCII] + data[size] + CRC32[4, BE] */
        uint32_t block_size = readU32Be_(buffer.data() + offset);
        offset += 4;

        if (offset + 4 > buffer.size()) break;
        uint32_t block_type = readU32Le_(buffer.data() + offset);
        offset += 4;

        if (offset + block_size > buffer.size()) break;

        ZiPatchBlock block;
        block.type = block_type;
        block.size = block_size;
        block.data.assign(buffer.data() + offset, buffer.data() + offset + block_size);
        block.file_offset = offset - 8;  /* Block start (size field position) */
        offset += block_size;

        /* Read CRC32 */
        if (offset + 4 <= buffer.size()) {
            block.crc = readU32Be_(buffer.data() + offset);
            offset += 4;

            /* CRC32 covers type + data */
            std::vector<uint8_t> crc_buf(4 + block.data.size());
            writeU32Le_(crc_buf.data(), block_type);
            std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
            uint32_t computed = Crc32Compute(crc_buf);
            block.crc_ok = (computed == block.crc);

            if (!*block.crc_ok) {
                m_crc_failures.push_back(m_blocks.size());
            }
        }

        /* Record FHDR information */
        if (block.IsFhdr() && m_result.empty()) {
            m_version = block.FhdrVersion();
            m_result = block.FhdrResult();
            m_declared_entries = block.FhdrEntryCount();
            m_declared_adddir = block.FhdrAdddirCount();
            m_declared_deldir = block.FhdrDeldirCount();
        }

        m_blocks.push_back(std::move(block));
    }
}

std::vector<const ZiPatchBlock*> ZiPatchFile::BlocksOf(uint32_t type) const {
    std::vector<const ZiPatchBlock*> result;
    for (auto& b : m_blocks) {
        if (b.type == type) result.push_back(&b);
    }
    return result;
}

std::string ZiPatchFile::Summary() const {
    std::ostringstream ss;
    ss << "ZiPatch file: " << m_filepath.string() << "\n";
    ss << "Version: " << m_version << " (0x" << std::hex << m_version << std::dec << ")\n";
    ss << "Result: " << m_result << "\n";
    ss << "Declared: entries=" << m_declared_entries
       << "  addDir=" << m_declared_adddir
       << "  delDir=" << m_declared_deldir << "\n";
    ss << "Blocks: " << m_blocks.size() << "\n";

    if (!m_crc_failures.empty()) {
        ss << "CRC failures: " << m_crc_failures.size()
           << " at blocks [";
        for (size_t i = 0; i < std::min(m_crc_failures.size(), size_t(10)); ++i) {
            if (i > 0) ss << ", ";
            ss << m_crc_failures[i];
        }
        if (m_crc_failures.size() > 10) ss << ", ...";
        ss << "]\n";
    }

    /* Block type statistics */
    std::map<uint32_t, size_t> counts;
    for (auto& b : m_blocks) {
        counts[b.type]++;
    }
    ss << "Block counts:\n";
    for (auto& [type, count] : counts) {
        ss << "  " << BlockTypeToString(type) << ": " << count << "\n";
    }

    return ss.str();
}

void ZiPatchBuilder::SetFhdr(uint32_t version, const std::string& result,
                              uint32_t num_entry, uint32_t num_adddir,
                              uint32_t num_deldir) {
    ZiPatchBlock block;
    block.type = BLOCK_FHDR;
    block.data.resize(20);
    auto* p = block.data.data();
    writeU32Be_(p, version);      p += 4;
    std::memcpy(p, result.c_str(), std::min(result.size(), size_t(4)));
    /* Zero-pad to 4 bytes if shorter */
    for (size_t i = result.size(); i < 4; ++i) p[i] = 0;
    p += 4;
    writeU32Be_(p, num_entry);    p += 4;
    writeU32Be_(p, num_adddir);   p += 4;
    writeU32Be_(p, num_deldir);
    block.size = 20;

    /* Compute CRC */
    std::vector<uint8_t> crc_buf(4 + block.data.size());
    writeU32Le_(crc_buf.data(), block.type);
    std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
    block.crc = Crc32Compute(crc_buf);
    block.crc_ok = true;

    m_fhdr = std::move(block);
}

void ZiPatchBuilder::AddApfs(uint64_t total_file_size, uint64_t total_disk_size) {
    ZiPatchBlock block;
    block.type = BLOCK_APFS;
    block.data.resize(16);
    writeU32Be_(block.data.data(), static_cast<uint32_t>(total_file_size >> 32));
    writeU32Be_(block.data.data() + 4, static_cast<uint32_t>(total_file_size));
    writeU32Be_(block.data.data() + 8, static_cast<uint32_t>(total_disk_size >> 32));
    writeU32Be_(block.data.data() + 12, static_cast<uint32_t>(total_disk_size));
    block.size = 16;

    std::vector<uint8_t> crc_buf(4 + block.data.size());
    writeU32Le_(crc_buf.data(), block.type);
    std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
    block.crc = Crc32Compute(crc_buf);
    block.crc_ok = true;

    m_blocks.push_back(std::move(block));
}

void ZiPatchBuilder::AddAply(uint32_t option_type, bool enabled) {
    ZiPatchBlock block;
    block.type = BLOCK_APLY;
    block.data.resize(12);
    writeU32Be_(block.data.data(), option_type);
    writeU32Be_(block.data.data() + 4, 4u);  /** Reserved field */
    writeU32Be_(block.data.data() + 8, enabled ? 1u : 0u);
    block.size = 12;

    std::vector<uint8_t> crc_buf(4 + block.data.size());
    writeU32Le_(crc_buf.data(), block.type);
    std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
    block.crc = Crc32Compute(crc_buf);
    block.crc_ok = true;

    m_blocks.push_back(std::move(block));
}

void ZiPatchBuilder::AddAdir(const std::string& path) {
    ZiPatchBlock block;
    block.type = BLOCK_ADIR;
    block.data.resize(4 + path.size());
    writeU32Be_(block.data.data(), static_cast<uint32_t>(path.size()));
    std::memcpy(block.data.data() + 4, path.data(), path.size());
    block.size = static_cast<uint32_t>(block.data.size());

    std::vector<uint8_t> crc_buf(4 + block.data.size());
    writeU32Le_(crc_buf.data(), block.type);
    std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
    block.crc = Crc32Compute(crc_buf);
    block.crc_ok = true;

    m_blocks.push_back(std::move(block));
}

void ZiPatchBuilder::AddDeld(const std::string& path) {
    ZiPatchBlock block;
    block.type = BLOCK_DELD;
    block.data.resize(4 + path.size());
    writeU32Be_(block.data.data(), static_cast<uint32_t>(path.size()));
    std::memcpy(block.data.data() + 4, path.data(), path.size());
    block.size = static_cast<uint32_t>(block.data.size());

    std::vector<uint8_t> crc_buf(4 + block.data.size());
    writeU32Le_(crc_buf.data(), block.type);
    std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
    block.crc = Crc32Compute(crc_buf);
    block.crc_ok = true;

    m_blocks.push_back(std::move(block));
}

void ZiPatchBuilder::AddEtry(const std::string& path,
                              const std::vector<ZiPatchChunk>& chunks) {
    ZiPatchBlock block;
    block.type = BLOCK_ETRY;

    /* Path prefix: pathLen[4, BE] + path + chunkCount[4, BE] */
    size_t prefix_size = 4 + path.size() + 4;
    size_t chunks_size = 0;
    for (auto& c : chunks) {
        chunks_size += CHUNK_HEADER_SIZE + c.data.size();
    }
    block.data.resize(prefix_size + chunks_size);

    auto* p = block.data.data();
    writeU32Be_(p, static_cast<uint32_t>(path.size())); p += 4;
    std::memcpy(p, path.data(), path.size()); p += path.size();
    writeU32Be_(p, static_cast<uint32_t>(chunks.size())); p += 4;

    for (auto& c : chunks) {
        writeU32Le_(p, c.mode);                          p += 4;
        std::memcpy(p, c.prev_hash.data(), SHA1_HASH_SIZE); p += SHA1_HASH_SIZE;
        std::memcpy(p, c.next_hash.data(), SHA1_HASH_SIZE); p += SHA1_HASH_SIZE;
        writeU32Le_(p, c.compression);                   p += 4;
        writeU32Be_(p, c.compressed_size);               p += 4;
        writeU32Be_(p, c.prev_size);                     p += 4;
        writeU32Be_(p, c.next_size);                     p += 4;
        if (!c.data.empty()) {
            std::memcpy(p, c.data.data(), c.data.size());
            p += c.data.size();
        }
    }

    block.size = static_cast<uint32_t>(block.data.size());

    std::vector<uint8_t> crc_buf(4 + block.data.size());
    writeU32Le_(crc_buf.data(), block.type);
    std::memcpy(crc_buf.data() + 4, block.data.data(), block.data.size());
    block.crc = Crc32Compute(crc_buf);
    block.crc_ok = true;

    m_blocks.push_back(std::move(block));
}

void ZiPatchBuilder::Write(const std::filesystem::path& filepath) const {
    auto data = ToBytes();
    std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open output file: " + filepath.string());
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
}

std::vector<uint8_t> ZiPatchBuilder::ToBytes() const {
    std::vector<uint8_t> out;
    /* Magic */
    out.insert(out.end(), std::begin(ZIPATCH_MAGIC), std::end(ZIPATCH_MAGIC));
    /** FHDR */
    if (m_fhdr) {
        m_fhdr->WriteTo(out, true);
    }
    /* Other blocks */
    for (auto& b : m_blocks) {
        b.WriteTo(out, true);
    }
    return out;
}


std::string ApplyResult::Summary() const {
    std::ostringstream ss;
    ss << "Patch apply summary:\n"
       << "  Files:     " << TotalFiles()
       << "  (added=" << files_added
       << " modified=" << files_modified
       << " deleted=" << files_deleted << ")\n"
       << "  Dirs:      " << (dirs_created + dirs_deleted)
       << "  (created=" << dirs_created
       << " deleted=" << dirs_deleted << ")\n"
       << "  Bytes:     " << bytes_written << " written\n"
       << "  Skipped:   " << skipped << "\n"
       << "  Errors:    " << errors.size();
    return ss.str();
}


void ZiPatchApplier::backupIfNeeded_(const std::filesystem::path& filepath,
                                       const std::filesystem::path& backup_dir) {
    if (backup_dir.empty() || !std::filesystem::exists(filepath)) return;
    auto rel = filepath.lexically_relative(
        filepath.root_path().empty() ? std::filesystem::current_path().root_path()
                                     : filepath.root_path());
    auto dest = backup_dir / rel;
    std::filesystem::create_directories(dest.parent_path());
    std::filesystem::copy_file(filepath, dest,
                               std::filesystem::copy_options::overwrite_existing);
}

ApplyResult ZiPatchApplier::Apply(
    const std::filesystem::path& install_dir,
    bool dry_run,
    bool no_verify,
    const std::filesystem::path& backup_dir,
    std::function<void(size_t, size_t)> progress)
{
    ApplyResult result;
    auto abs_install = std::filesystem::absolute(install_dir);
    std::array<uint8_t, SHA1_HASH_SIZE> null_hash{};

    size_t total = m_patch.BlockCount();
    for (size_t i = 0; i < total; ++i) {
        if (progress) progress(i, total);

        const auto& block = m_patch.Blocks()[i];

        if (block.IsAdir()) {
            auto rel = block.Path();
            /* Convert path separators */
            std::replace(rel.begin(), rel.end(), '\\', '/');
            auto full = abs_install / rel;

            if (dry_run) {
                result.dirs_created++;
            } else {
                std::error_code ec;
                std::filesystem::create_directories(full, ec);
                if (ec) {
                    result.errors.push_back("ADIR:" + rel + ":" + ec.message());
                } else {
                    result.dirs_created++;
                }
            }
        }
        else if (block.IsDeld()) {
            auto rel = block.Path();
            std::replace(rel.begin(), rel.end(), '\\', '/');
            auto full = abs_install / rel;

            if (dry_run) {
                result.dirs_deleted++;
            } else {
                std::error_code ec;
                if (std::filesystem::is_directory(full, ec) && !ec) {
                    std::filesystem::remove(full, ec);
                    if (ec) {
                        result.errors.push_back("DELD:" + rel + ":" + ec.message());
                    } else {
                        result.dirs_deleted++;
                    }
                } else {
                    result.skipped++;
                }
            }
        }
        else if (block.IsEtry()) {
            auto rel = block.Path();
            std::replace(rel.begin(), rel.end(), '\\', '/');
            auto full = abs_install / rel;
            auto chunks = block.Chunks();

            for (auto& chunk : chunks) {
                std::string mode_name = chunk.ModeName();

                try {
                    auto decompressed = chunk.Decompress();

                    /* Verify next_hash of decompressed data */
                    if (chunk.next_hash != null_hash) {
                        Sha1 sha1;
                        sha1.Update(decompressed);
                        auto actual = sha1.Finalize();
                        if (std::memcmp(actual.data(), chunk.next_hash.data(),
                                        SHA1_HASH_SIZE) != 0) {
                            result.errors.push_back(
                                rel + ":next_hash mismatch");
                            continue;
                        }
                    }

                    if (chunk.mode == CHUNK_MODE_ADD) {
                        /* Verify old file does not exist (when prev_hash is non-zero) */
                        if (!no_verify && chunk.prev_hash != null_hash) {
                            if (std::filesystem::exists(full)) {
                                result.errors.push_back(
                                    rel + ":file exists for add");
                                continue;
                            }
                        }

                        if (dry_run) {
                            result.files_added++;
                            result.bytes_written += decompressed.size();
                        } else {
                            std::filesystem::create_directories(full.parent_path());
                            backupIfNeeded_(full, backup_dir);
                            std::ofstream of(full, std::ios::binary | std::ios::trunc);
                            of.write(reinterpret_cast<const char*>(decompressed.data()),
                                     static_cast<std::streamsize>(decompressed.size()));
                            result.files_added++;
                            result.bytes_written += decompressed.size();
                        }
                    }
                    else if (chunk.mode == CHUNK_MODE_DELETE) {
                        if (!dry_run && !no_verify && chunk.prev_hash != null_hash) {
                            if (std::filesystem::exists(full)) {
                                std::ifstream inf(full, std::ios::binary);
                                std::vector<uint8_t> existing(
                                    (std::istreambuf_iterator<char>(inf)),
                                    std::istreambuf_iterator<char>());
                                Sha1 sha1;
                                sha1.Update(existing);
                                auto h = sha1.Finalize();
                                if (std::memcmp(h.data(), chunk.prev_hash.data(),
                                                SHA1_HASH_SIZE) != 0) {
                                    result.errors.push_back(
                                        rel + ":hash mismatch before delete");
                                    continue;
                                }
                            }
                        }

                        if (dry_run) {
                            result.files_deleted++;
                        } else {
                            backupIfNeeded_(full, backup_dir);
                            std::error_code ec;
                            std::filesystem::remove(full, ec);
                            if (!ec) result.files_deleted++;
                            else result.skipped++;
                        }
                    }
                    else if (chunk.mode == CHUNK_MODE_MODIFY) {
                        if (!dry_run && !no_verify) {
                            if (!std::filesystem::exists(full)) {
                                result.errors.push_back(
                                    rel + ":file not found for modify");
                                continue;
                            }
                            if (chunk.prev_hash != null_hash) {
                                std::ifstream inf(full, std::ios::binary);
                                std::vector<uint8_t> existing(
                                    (std::istreambuf_iterator<char>(inf)),
                                    std::istreambuf_iterator<char>());
                                Sha1 sha1;
                                sha1.Update(existing);
                                auto h = sha1.Finalize();
                                if (std::memcmp(h.data(), chunk.prev_hash.data(),
                                                SHA1_HASH_SIZE) != 0) {
                                    result.errors.push_back(
                                        rel + ":hash mismatch before modify");
                                    continue;
                                }
                            }
                        }

                        if (dry_run) {
                            result.files_modified++;
                            result.bytes_written += decompressed.size();
                        } else {
                            backupIfNeeded_(full, backup_dir);
                            std::ofstream of(full, std::ios::binary | std::ios::trunc);
                            of.write(reinterpret_cast<const char*>(decompressed.data()),
                                     static_cast<std::streamsize>(decompressed.size()));
                            result.files_modified++;
                            result.bytes_written += decompressed.size();
                        }
                    }
                    else {
                        result.skipped++;
                    }
                } catch (const std::exception& e) {
                    result.errors.push_back(std::string(rel) + ":" + e.what());
                }
            }
        }
    }

    if (progress) progress(total, total);
    return result;
}
