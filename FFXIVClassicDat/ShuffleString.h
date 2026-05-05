#pragma once
#include <cstdint>

/**
 * @brief ShuffleString encryption/decryption utility.
 *
 * Used for XML files and other data that employ this encryption method.
 * Note: re-packaging does not require encryption; the game can read plaintext.
 */
class ShuffleString
{
public:
	/**
	 * @brief Decrypt shuffle-encrypted data.
	 * @param srcData Source buffer
	 * @param srcLen Length of source data
	 * @param dstData Destination buffer
	 * @param dstLen Length of destination buffer
	 * @return Number of successfully processed bytes on success, negative value on failure
	 */
	int Decrypt(void *srcData, int srcLen, void *dstData, int dstLen);

	/**
	 * @brief Encrypt data using shuffle encryption.
	 * @param srcData Source buffer
	 * @param srcLen Length of source data
	 * @param dstData Destination buffer
	 * @param dstLen Length of destination buffer
	 * @return Number of successfully processed bytes on success, negative value on failure
	 */
	int Encrypt(void *srcData, int srcLen, void *dstData, int dstLen);

protected:
	/**
	 * @brief Shuffle characters by swapping pairs from head/tail inward.
	 * @param dst Buffer to shuffle in-place
	 * @param length Number of bytes to shuffle
	 */
	void Shuffle(void *dst, int length);

	/**
	 * @brief Derive two 16-bit XOR factors from a key.
	 * @param key The key value
	 * @param a Output: first factor
	 * @param b Output: second factor
	 */
	void GetFactors(int16_t key, uint16_t *a, uint16_t *b);
};
