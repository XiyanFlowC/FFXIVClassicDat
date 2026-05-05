#include "SimpleString.h"

#include <cstdint>

int SimpleString::Decrypt(void *srcData, int srcLen, void *dstData, int dstLen)
{
	uint8_t *src = (uint8_t *)srcData;
	uint8_t *dst = (uint8_t *)dstData;

	/* Not starting with 0xFF, not a simple string ciphertext */
	if (!srcData || *src != 0xFF)
	{
		return -1;
	}
	if (!dstData || dstLen < srcLen - 1)
	{
		return -2;
	}
	uint8_t *end = src + srcLen;

	/* Skip the initial 0xFF marker */
	++src;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x73;
	}
	return srcLen - 1;
}

int SimpleString::Encrypt(void *srcData, int srcLen, void *dstData, int dstLen)
{
	uint8_t *src = (uint8_t *)srcData;
	uint8_t *dst = (uint8_t *)dstData;

	/* Already starts with 0xFF, already simple string ciphertext */
	if (!srcData || *src == 0xFF)
	{
		return -1;
	}
	if (!dstData || dstLen < srcLen + 1)
	{
		return -2;
	}
	if (srcData == dstData)
	{
		return -3;
	}
	uint8_t *end = src + srcLen;

	/* Write the marker */
	*dst++ = (uint8_t)0xFF;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x73;
	}
	return srcLen + 1;
}
