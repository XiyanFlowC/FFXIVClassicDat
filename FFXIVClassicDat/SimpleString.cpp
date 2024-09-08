#include "SimpleString.h"

int SimpleString::Decrypt(void *p_src, int p_srcLeng, void *p_dst, int p_dstLeng)
{
	char *src = (char *)p_src;
	char *dst = (char *)p_dst;
	// �ʼ����0xFF�����Ǽ��ַ�������
	if (!p_src || *src != 0xFF)
	{
		return -1;
	}
	if (!p_dst || p_dstLeng < p_srcLeng - 1)
	{
		return -2;
	}
	char *end = src + p_srcLeng;
	// �����ʼ�ı�־ 0xFF
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
	// �ʼ��0xFF�����Ǽ��ַ�������
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
	// д���־
	*dst++ = 0xFF;
	while (src < end)
	{
		*dst++ = *src++ ^ 0x75;
	}
	return p_srcLeng + 1;
}
