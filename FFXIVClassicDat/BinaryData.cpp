#include "BinaryData.h"

BinaryData::BinaryData()
    : m_length(0)
{
}

BinaryData::BinaryData(void *p_data, size_t p_length, bool p_duplicate)
{
	SetData(p_data, p_length, p_duplicate);
}

BinaryData::BinaryData(size_t p_length)
{
    m_data = std::shared_ptr<char[]>(new char[p_length]);
    m_length = p_length;
}

void *BinaryData::GetData() const noexcept
{
	return m_data.get();
}

size_t BinaryData::GetLength() const noexcept
{
	return m_length;
}

void BinaryData::SetData(void *p_data, size_t p_length, bool p_duplicate)
{
    if (p_duplicate)
    {
        m_data = std::shared_ptr<char[]>(new char[p_length]);
        memcpy(m_data.get(), p_data, p_length);
    }
    else
    {
        m_data = std::shared_ptr<char[]>((char *)p_data);
    }
    m_length = p_length;
}

void BinaryData::SetData(const void *p_data, size_t p_length)
{
    SetData((char *)p_data, p_length, true);
}
