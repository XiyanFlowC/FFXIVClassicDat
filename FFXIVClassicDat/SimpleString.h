#pragma once

class SimpleString
{
public:
	int Decrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng);
	int Encrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng);
};

