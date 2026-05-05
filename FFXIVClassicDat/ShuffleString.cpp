#include "ShuffleString.h"

#include <cstring>
#include <assert.h>

int ShuffleString::Decrypt(void *srcData, int srcLen, void *dstData, int dstLen)
{
	char *src = (char *)srcData;

	/* ShuffleString encryption marker: last byte is 0xF1 (-15).
	 *  Everything before the marker is the encrypted string data. */
	if (!src || src[srcLen - 1] != -15)
	{
		return -1;
	}
	if (!dstData)
	{
		return -2;
	}
	if (srcLen - 1 > dstLen)
	{
		return -3;
	}
	if (srcData != dstData)
		memcpy(dstData, srcData, srcLen - 1);

	Shuffle(dstData, srcLen - 1);
	uint16_t a, b;
	GetFactors(srcLen - 1, &a, &b);
	char *cur = (char *)dstData, *end = ((char *)dstData) + srcLen - 1;
	
	while (cur < end)
	{
		*((uint16_t *)cur) ^= a;
		cur += 4;
	}

	cur = ((char *)dstData) + 2;
	while (cur < end)
	{
		*((uint16_t *)cur) ^= b;
		cur += 4;
	}

	if ((srcLen - 1) & 1)
	{
		*(end - 1) ^= (uint8_t)(b & 0xFF);
	}

	return srcLen - 1;
}

int ShuffleString::Encrypt(void *srcData, int srcLen, void *dstData, int dstLen)
{
	/* This implementation has not been fully verified. */
	char *src = (char *)srcData;
	char *dst = (char *)dstData;

	/* ShuffleString encryption marker: last byte is 0xF1 (-15).
	 *  Everything before the marker is the encrypted string data. */
	if (!src || src[srcLen - 1] == -15)
	{
		return -1;
	}
	if (!dstData)
	{
		return -2;
	}
	if (srcLen - 1 > dstLen)
	{
		return -3;
	}

	if (srcData != dstData)
		memcpy(dstData, srcData, srcLen);

	uint16_t a, b;
	GetFactors(srcLen, &a, &b);

	char *cur = (char *)dstData + 2, *end = (char *)dstData + srcLen;
		
	while (cur < end)
	{
		*((uint16_t *)cur) ^= b;
		cur += 4;
	}

	cur = (char *)dstData;
	while (cur < end)
	{
		*((uint16_t *)cur) ^= a;
		cur += 4;
	}

	if (srcLen & 1)
	{
		*(end - 1) ^= (uint8_t)(b & 0xFF);
	}

	/* Shuffle the data */
	Shuffle(dstData, srcLen);

	/* Set the marker byte */
	((char *)dstData)[srcLen] = -15;

	return srcLen + 1;
}

void ShuffleString::Shuffle(void *dst, int length)
{
	char *rcur = ((char *)dst + length - 1), *cur = (char *)dst;
	while (cur < rcur)
	{
		/* Swap head/tail pair */
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
