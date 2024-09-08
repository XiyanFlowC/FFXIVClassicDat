#pragma once

#include <cstdint>

class Sheet
{
public:
	Sheet(uint32_t infofileId);

protected:
	uint32_t m_infofileId;
	uint32_t m_enableId;
	uint32_t m_indexId;
	uint32_t m_dataId;
private:
};

