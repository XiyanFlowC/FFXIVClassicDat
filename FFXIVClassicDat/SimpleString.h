#pragma once

class SimpleString
{
public:
	int Decrypt(void *srcData, int srcLen, void *dstData, int dstLen);
	int Encrypt(void *srcData, int srcLen, void *dstData, int dstLen);
};
