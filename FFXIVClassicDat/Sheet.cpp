#include "Sheet.h"

#include "DataManager.h"
#include "ShuffleString.h"


Sheet::Sheet(const std::u8string &name, int columnMax, int columnCount, int cache)
	: m_name(name), m_columnCount(columnCount), m_columnMax(columnMax), m_cache(cache)
{
}

const std::u8string &Sheet::GetName() const
{
	return m_name;
}
