#pragma once

#include <cstdint>

/**
 * @brief BlowFish decryption utility.
 *
 * Reverse-engineered from the game binary.
 * 2024/9/8 - This is actually a standard BlowFish implementation.
 * Refactored with proper naming for readability.
 */
class BlowFish
{
public:
	uint8_t m_pbox[72];
	uint8_t m_sbox[0x1000];

	BlowFish(const char *phrase, int keyLength);

	BlowFish *MakeKey(const char *phrase, int keyLength);

	void Decrypt(void *dst, void *src, size_t length);

	class BlowFishKeyBox
	{
    public:
		/**
		 * @brief Default S-box table (ffxivboot.exe:0xFB9FE0).
		 */
        static uint8_t sbox[0x1000];

		/**
		 * @brief Default P-box table (ffxivboot.exe:0xFB9F98).
		 */
		static uint8_t pbox[72];
	};
private:
	void EncryptCell(uint32_t *mod1, uint32_t *mod2);

	void DecryptCell(uint32_t *mod1, uint32_t *mod2);
};
