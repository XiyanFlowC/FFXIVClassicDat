#include "BinaryData.h"

BinaryData::BinaryData()
    : length(0)
{
}

BinaryData::BinaryData(void *data, size_t length, bool duplicate)
{
	SetData(data, length, duplicate);
}

BinaryData::BinaryData(size_t length)
{
    this->data = std::shared_ptr<char[]>(new char[length]);
    this->length = length;
}

void *BinaryData::GetData() const noexcept
{
	return data.get();
}

size_t BinaryData::GetLength() const noexcept
{
	return length;
}

void BinaryData::SetData(void *data, size_t length, bool duplicate)
{
    if (duplicate)
    {
        this->data = std::shared_ptr<char[]>(new char[length]);
        memcpy(this->data.get(), data, length);
    }
    else
    {
        this->data = std::shared_ptr<char[]>((char *)data);
    }
    this->length = length;
}

void BinaryData::SetData(const void *data, size_t length)
{
    SetData((char *)data, length, true);
}
