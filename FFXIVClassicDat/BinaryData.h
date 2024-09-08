#pragma once

#include <memory>
#include <cstdint>

class BinaryData
{
	std::shared_ptr<char[]> data;
	size_t length;

public:

	BinaryData();

	/**
	 * @brief 初始化二进制数据。
	 * @param data 数据所在缓冲区。
	 * @param length 数据长度。
	 * @param duplicate 是否复制，若为否，则将传入的data直接托管（shared_ptr）。
	*/
	BinaryData(void *data, size_t length, bool duplicate = true);

	/**
	 * @brief 设定空的缓冲区。
	 * @param length 
	 */
	BinaryData(size_t length);

	~BinaryData() {};

	void *GetData() const noexcept;

	size_t GetLength() const noexcept;

	/**
	 * @brief 管理对应数据
	 * @param data 数据
	 * @param length 数据长度
	 * @param duplicate 是否复制（不对源操作）
	*/
	void SetData(void *data, size_t length, bool duplicate = true);

	/**
	 * @brief 管理对应数据
	 * @param data 数据
	 * @param length 数据长度
	*/
	void SetData(const void *data, size_t length);
};

