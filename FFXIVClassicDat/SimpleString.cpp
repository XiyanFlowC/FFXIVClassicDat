#include "SimpleString.h"

int SimpleString::Decrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng)
{
	char *src = (char *)p_src;
	char *dst = (char *)p_dst;
	// 最开始不是0xFF，不是简单字符串密文
	if (!p_src || *src != 0xFF)
	{
		return -1;
	}
	if (!p_dst || p_dstLeng < p_srcLeng - 1)
	{
		return -2;
	}
	char *end = src + p_srcLeng;
	// 跳过最开始的标志 0xFF
	++src;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x75;
	}
	return p_srcLeng - 1;
}

int SimpleString::Encrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng)
{
	char *src = (char *)p_src;
	char *dst = (char *)p_dst;
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
	char *end = src + p_srcLeng;
	// 写入标志
	*dst++ = (char)0xFF;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x75;
	}
	return p_srcLeng + 1;
}
