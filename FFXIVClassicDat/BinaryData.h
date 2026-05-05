#pragma once

#include <memory>
#include <cstdint>

/**
 * @brief 表示二进制数据的类，用于管理任意字节序列。
 */
class BinaryData
{
	std::shared_ptr<char[]> m_data;
	size_t m_length;

public:

	/**
	 * @brief 构造一个空的二进制数据对象。
	 */
	BinaryData();

	/**
	 * @brief 初始化二进制数据。
	 * @param p_data 数据所在缓冲区。
	 * @param p_length 数据长度。
	 * @param p_duplicate 是否复制，若为否，则将传入的data直接托管（shared_ptr）。
	 */
	BinaryData(void *p_data, size_t p_length, bool p_duplicate = true);

	/**
	 * @brief 初始化空数据与预分配的缓冲区。
	 * @param p_length 要分配的字节数，数据内容未定义。
	 */
	BinaryData(size_t p_length);

	~BinaryData() {};

	/**
	 * @brief 获取数据指针。
	 * @return 指向数据的指针。
	 */
	void *GetData() const noexcept;

	/**
	 * @brief 获取长度。
	 * @return 长度值。
	 */
	size_t GetLength() const noexcept;

	/**
	 * @brief 设置对应数据
	 * @param p_data 数据
	 * @param p_length 数据长度
	 * @param p_duplicate 是否复制（不对源操作）
	 */
	void SetData(void *p_data, size_t p_length, bool p_duplicate = true);

	/**
	 * @brief 设置对应数据（只读版本）。
	 * @param p_data 数据指针。
	 * @param p_length 数据长度。
	 */
	void SetData(const void *p_data, size_t p_length);
};
