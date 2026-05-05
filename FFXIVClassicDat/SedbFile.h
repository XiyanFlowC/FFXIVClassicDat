#pragma once

#include <cstdint>

class SedbFile
{
public:
	/**
	 * @brief Initialize a SEDB file container from memory.
	 * @param buffer A buffer containing SEDB file raw layout.
	 */
	SedbFile(const char *buffer);

	/**
	 * @brief Query and load a file from DataManager as a SEDB.
	 * @param fileId The file ID.
	 */
	SedbFile(uint32_t fileId);

	const char* GetRawData() const { return m_data; }

	const char* GetPayload() const { return m_payload; }

	const char* GetType() const { return m_data + 4; }

#pragma pack(push, 1)
	struct SedbHeader {
		char magic[4];   /* "SEDB" */
		char typestr[4]; /* "lyb\0" "SSCF" etc. */
		int32_t type;    /* Unknown, 3 for SSCF and 8 for lyb */
		int16_t type2;   /* Unknown, 0x400 for SSCF and 0 for lyb */
		int16_t headersize; /* Unknown, always 0x30 */
		int32_t fileSize;   /* Total size of the file */
		int32_t reserved[7];
	};
#pragma pack(pop)

protected:
	void Load(const char* buffer);

private:
	char* m_data;
	char* m_payload;
};
