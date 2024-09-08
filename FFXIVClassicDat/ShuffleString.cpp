#include "ShuffleString.h"

#include <cstring>
#include <assert.h>

int ShuffleString::Decrypt(void *p_src, int srcLeng, void *p_dst, int dstLeng)
{
	char *src = (char *)p_src;
	// ShuffleString 加密标志：最后一个字节为-15（0xF1
	// 标志以外为加密的字符串数据
	if (!src || src[srcLeng - 1] != -15)
	{
		// 非加密数据
		return -1;
	}
	if (!p_dst)
	{
		return -2;
	}
	if (srcLeng - 1 > dstLeng)
	{
		return -3;
	}
	if (p_src != p_dst)
		memcpy(p_dst, p_src, srcLeng - 1);

	Shuffle(p_dst, srcLeng - 1);
	uint16_t a, b;
	GetFactors(srcLeng - 1, &a, &b);
	char *cur = (char *)p_dst, *end = ((char *)p_dst) + srcLeng - 1;
	
	while (cur < end)
	{
		*((uint16_t *)cur) ^= a;
		cur += 4;
	}

	cur = ((char *)p_dst) + 2;
	while (cur < end)
	{
		*((uint16_t *)cur) ^= b;
		cur += 4;
	}

	if ((srcLeng - 1) & 1)
	{
		*(end - 1) ^= (uint8_t)(b & 0xFF);
	}

	return srcLeng - 1;
}

int ShuffleString::Encrypt(void *p_src, int srcLeng, void *p_dst, int dstLeng)
{
	// 该实现尚未确认其有效性。
	char *src = (char *)p_src;
	char *dst = (char *)p_dst;
	// ShuffleString 加密标志：最后一个字节为-15（0xF1
	// 标志以外为加密的字符串数据
	if (!src || src[srcLeng - 1] == -15)
	{
		// 已加密数据
		return -1;
	}
	if (!p_dst)
	{
		return -2;
	}
	if (srcLeng - 1 > dstLeng)
	{
		return -3;
	}

	if (p_src != p_dst)
		memcpy(p_dst, p_src, srcLeng);

	uint16_t a, b;
	GetFactors(srcLeng, &a, &b);

	char *cur = (char *)p_dst + 2, *end = (char *)p_dst + srcLeng;
		
	while (cur < end)
	{
		*((uint16_t *)cur) ^= b;
		cur += 4;
	}

	cur = (char *)p_dst;
	while (cur < end)
	{
		*((uint16_t *)cur) ^= a;
		cur += 4;
	}

	if (srcLeng & 1)
	{
		*(end - 1) ^= (uint8_t)(b & 0xFF);
	}

	// 混淆
	Shuffle(p_dst, srcLeng);

	// 设置标志位
	((char *)p_dst)[srcLeng] = -15;

	return srcLeng + 1;
}

void ShuffleString::Shuffle(void *dst, int length)
{
	char *rcur = ((char *)dst + length - 1), *cur = (char *)dst;
	while (cur < rcur)
	{
		// 首尾隔位交换
		char t = *rcur;
		*rcur = *cur;
		*cur = t;
		cur += 2;
		rcur -= 2;
	}
}

void ShuffleString::GetFactors(int16_t key, uint16_t *a, uint16_t *b)
{
	*a = 7 * key;
	*b = ~(uint16_t)(((*a) + 1) >> (((*a) / 3) & 3));
}
