#include "SimpleString.h"

#include <cstdint>

int SimpleString::Decrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng)
{
	uint8_t *src = (uint8_t *)p_src;
	uint8_t *dst = (uint8_t *)p_dst;
	// 最开始不是0xFF，不是简单字符串密文
	if (!p_src || *src != 0xFF)
	{
		return -1;
	}
	if (!p_dst || p_dstLeng < p_srcLeng - 1)
	{
		return -2;
	}
	uint8_t *end = src + p_srcLeng;
	// 跳过最开始的标志 0xFF
	++src;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x73;
	}
	return p_srcLeng - 1;
}

int SimpleString::Encrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng)
{
	uint8_t *src = (uint8_t *)p_src;
	uint8_t *dst = (uint8_t *)p_dst;
	// 最开始是0xFF，已是简单字符串密文
	if (!p_src || *src == 0xFF)
	{
		return -1;
	}
	if (!p_dst || p_dstLeng < p_srcLeng + 1)
	{
		return -2;
	}
	if (p_src == p_dst)
	{
		return -3;
	}
	uint8_t *end = src + p_srcLeng;
	// 写入标志
	*dst++ = (uint8_t)0xFF;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x73;
	}
	return p_srcLeng + 1;
}
