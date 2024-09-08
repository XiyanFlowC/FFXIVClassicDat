#pragma once
#include <cstdint>

/**
 * @brief ShuffleString 加解密
 * XML 文件等，使用此种方式加密（注：再封装时不需加密，游戏可读取明文信息）
 */
class ShuffleString
{
public:
	/**
	 * @brief 解密
	 * @param src 来源
	 * @param srcLeng 来源长度
	 * @param dst 目的
	 * @param dstLeng 目的长度
	 * @return 成功返回成功处理的字符串数目，否则返回负数
	 */
	int Decrypt(void *src, int srcLeng, void *dst, int dstLeng);

	/**
	 * @brief 加密
	 * @param src 
	 * @param srcLeng 
	 * @param dst 
	 * @param dstLeng 
	 * @return 
	 */
	int Encrypt(void *src, int srcLeng, void *dst, int dstLeng);

protected:
	void Shuffle(void *dst, int length);

	void GetFactors(int16_t key, uint16_t *a, uint16_t *b);
};

