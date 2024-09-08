#pragma once

#include <cstdint>

/**
 * @brief 解密函数，从逆向结果恩抄的
 * 2024/9/8 喵的是BlowFish
 */
class SqwtDecryptUtility
{
public:
	uint8_t pbox[72];
	uint8_t sbox[0x1000];

	SqwtDecryptUtility(const char *phrase, int keyLength);

	SqwtDecryptUtility *MakeKey(const char *phrase, int keyLength);

	void Decrypt(void *dst, void *src, size_t length);

	class SqwtKeyStore
	{
    public:
		/**
		 * @brief ffxivboot.exe:FB9FE0
		 */
        static uint8_t sbox[0x1000];

		/**
		 * @brief ffxivboot.exe:FB9F98
		 */
		static uint8_t pbox[72];
	};
private:
	void EncryptCell(uint32_t *mod1, uint32_t *mod2);

	void DecryptCell(uint32_t *mod1, uint32_t *mod2);
};

