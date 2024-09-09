#pragma once

#include <cstdint>

/**
 * @brief 解密，从逆向结果恩抄的
 * 2024/9/8 喵的是BlowFish - 重构，重命名很花时间，所以只要知道这个实际上是BlowFish的标准实现就好了。
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

