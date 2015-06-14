// From http://github.com/b1l1s/ctr

#include "crypto.h"

#include <stddef.h>
#include <memfuncs.h>
/* original version by megazig */

#ifndef __thumb__
#define BSWAP32(x) {\
	__asm__\
	(\
		"eor r1, %1, %1, ror #16\n\t"\
		"bic r1, r1, #0xFF0000\n\t"\
		"mov %0, %1, ror #8\n\t"\
		"eor %0, %0, r1, lsr #8\n\t"\
		:"=r"(x)\
		:"0"(x)\
		:"r1"\
	);\
};

#define ADD_u128_uint32_t(u128_0, u128_1, u128_2, u128_3, uint32_t_0) {\
__asm__\
	(\
		"adds %0, %4\n\t"\
		"addcss %1, %1, #1\n\t"\
		"addcss %2, %2, #1\n\t"\
		"addcs %3, %3, #1\n\t"\
		: "+r"(u128_0), "+r"(u128_1), "+r"(u128_2), "+r"(u128_3)\
		: "r"(uint32_t_0)\
		: "cc"\
	);\
}
#else
#define BSWAP32(x) {x = __builtin_bswap32(x);}

#define ADD_u128_uint32_t(u128_0, u128_1, u128_2, u128_3, uint32_t_0) {\
__asm__\
	(\
		"mov r4, #0\n\t"\
		"add %0, %0, %4\n\t"\
		"adc %1, %1, r4\n\t"\
		"adc %2, %2, r4\n\t"\
		"adc %3, %3, r4\n\t"\
		: "+r"(u128_0), "+r"(u128_1), "+r"(u128_2), "+r"(u128_3)\
		: "r"(uint32_t_0)\
		: "cc", "r4"\
	);\
}
#endif /*__thumb__*/

void aes_setkey(uint8_t keyslot, const void* key, uint32_t keyType, uint32_t mode)
{
	if(keyslot <= 0x03) return; // Ignore TWL keys for now

	uint32_t* key32 = (uint32_t*)key;
	*REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN | AES_CNT_INPUT_ORDER)) | mode;
	*REG_AESKEYCNT = (*REG_AESKEYCNT >> 6 << 6) | keyslot | AES_KEYCNT_WRITE;

	REG_AESKEYFIFO[keyType] = key32[0];
	REG_AESKEYFIFO[keyType] = key32[1];
	REG_AESKEYFIFO[keyType] = key32[2];
	REG_AESKEYFIFO[keyType] = key32[3];
}

void aes_use_keyslot(uint8_t keyslot)
{
	if(keyslot > 0x3F)
		return;

	*REG_AESKEYSEL = keyslot;
	*REG_AESCNT = *REG_AESCNT | 0x04000000; /* mystery bit */
}

void aes_setiv(const void* iv, uint32_t mode)
{
	const uint32_t* iv32 = (const uint32_t*)iv;
	*REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN | AES_CNT_INPUT_ORDER)) | mode;

	// Word order for IV can't be changed in REG_AESCNT and always default to reversed
	if(mode & AES_INPUT_NORMAL)
	{
		REG_AESCTR[0] = iv32[3];
		REG_AESCTR[1] = iv32[2];
		REG_AESCTR[2] = iv32[1];
		REG_AESCTR[3] = iv32[0];
	}
	else
	{
		REG_AESCTR[0] = iv32[0];
		REG_AESCTR[1] = iv32[1];
		REG_AESCTR[2] = iv32[2];
		REG_AESCTR[3] = iv32[3];
	}
}

void aes_advctr(void* ctr, uint32_t val, uint32_t mode)
{
	uint32_t* ctr32 = (uint32_t*)ctr;
	
	int i;
	if(mode & AES_INPUT_BE)
	{
		for(i = 0; i < 4; ++i) // Endian swap
			BSWAP32(ctr32[i]);
	}
	
	if(mode & AES_INPUT_NORMAL)
	{
		ADD_u128_uint32_t(ctr32[3], ctr32[2], ctr32[1], ctr32[0], val);
	}
	else
	{
		ADD_u128_uint32_t(ctr32[0], ctr32[1], ctr32[2], ctr32[3], val);
	}
	
	if(mode & AES_INPUT_BE)
	{
		for(i = 0; i < 4; ++i) // Endian swap
			BSWAP32(ctr32[i]);
	}
}

void aes_change_ctrmode(void* ctr, uint32_t fromMode, uint32_t toMode)
{
	uint32_t* ctr32 = (uint32_t*)ctr;
	int i;
	if((fromMode ^ toMode) & AES_CNT_INPUT_ENDIAN)
	{
		for(i = 0; i < 4; ++i)
			BSWAP32(ctr32[i]);
	}

	if((fromMode ^ toMode) & AES_CNT_INPUT_ORDER)
	{
		uint32_t temp = ctr32[0];
		ctr32[0] = ctr32[3];
		ctr32[3] = temp;

		temp = ctr32[1];
		ctr32[1] = ctr32[2];
		ctr32[2] = temp;
	}
}

void aes_batch(void* dst, const void* src, uint32_t blockCount)
{
	*REG_AESBLKCNT = blockCount << 16;
	*REG_AESCNT |=	AES_CNT_START;
	
	const uint32_t* src32	= (const uint32_t*)src;
	uint32_t* dst32			= (uint32_t*)dst;
	
	uint32_t wbc = blockCount;
	uint32_t rbc = blockCount;
	
	while(rbc)
	{
		if(wbc && ((*REG_AESCNT & 0x1F) <= 0xC)) // There's space for at least 4 ints
		{
			*REG_AESWRFIFO = *src32++;
			*REG_AESWRFIFO = *src32++;
			*REG_AESWRFIFO = *src32++;
			*REG_AESWRFIFO = *src32++;
			wbc--;
		}
		
		if(rbc && ((*REG_AESCNT & (0x1F << 0x5)) >= (0x4 << 0x5))) // At least 4 ints available for read
		{
			*dst32++ = *REG_AESRDFIFO;
			*dst32++ = *REG_AESRDFIFO;
			*dst32++ = *REG_AESRDFIFO;
			*dst32++ = *REG_AESRDFIFO;
			rbc--;
		}
	}
}

inline void aes_setmode(uint32_t mode)
{
	*REG_AESCNT =	mode |
					AES_CNT_INPUT_ORDER | AES_CNT_OUTPUT_ORDER |
					AES_CNT_INPUT_ENDIAN | AES_CNT_OUTPUT_ENDIAN |
					AES_CNT_FLUSH_READ | AES_CNT_FLUSH_WRITE;
}

void aes(void* dst, const void* src, uint32_t blockCount, void* iv, uint32_t mode, uint32_t ivMode)
{
	aes_setmode(mode);

	uint32_t blocks;
	while(blockCount != 0)
	{
		aes_setiv(iv, ivMode);

		blocks = (blockCount >= 0xFFFF) ? 0xFFFF : blockCount;

		// Save the last block for the next decryption CBC batch's iv
		if((mode & AES_CBC_DECRYPT_MODE) == AES_CBC_DECRYPT_MODE)
		{
			memcpy(iv, src + (blocks - 1) * AES_BLOCK_SIZE, AES_BLOCK_SIZE);
			aes_change_ctrmode(iv, AES_INPUT_BE | AES_INPUT_NORMAL, ivMode);
		}

		// Process the current batch
		aes_batch(dst, src, blocks);

		// Save the last block for the next encryption CBC batch's iv
		if((mode & AES_CBC_ENCRYPT_MODE) == AES_CBC_ENCRYPT_MODE)
		{
			memcpy(iv, dst + (blocks - 1) * AES_BLOCK_SIZE, AES_BLOCK_SIZE);
			aes_change_ctrmode(iv, AES_INPUT_BE | AES_INPUT_NORMAL, ivMode);
		}
		
		// Advance counter for CTR mode
		else if((mode & AES_CTR_MODE) == AES_CTR_MODE)
			aes_advctr(iv, blocks, ivMode);

		src += blocks * AES_BLOCK_SIZE;
		dst += blocks * AES_BLOCK_SIZE;
		blockCount -= blocks;
	}
}

void ncch_getctr(const ncch_h* ncch, uint8_t* ctr, uint8_t type)
{
	uint32_t version = ncch->version;
	const uint8_t* partitionID = ncch->partitionID;

	int i;
	for(i = 0; i < 16; i++)
		ctr[i] = 0x00;

	if(version == 2 || version == 0)
	{
		for(i=0; i<8; i++)
			ctr[i] = partitionID[7 - i]; // Convert to big endian & normal input
		ctr[8] = type;
	}
	else if(version == 1)
	{
		int x = 0;
		if(type == NCCHTYPE_EXHEADER)
			x = MEDIA_UNITS;
		else if(type == NCCHTYPE_EXEFS)
			x = ncch->exeFSOffset * MEDIA_UNITS;
		else if (type == NCCHTYPE_ROMFS)
			x = ncch->exeFSOffset * MEDIA_UNITS;

		for(i = 0; i < 8; i++)
			ctr[i] = partitionID[i];
		for(i = 0; i < 4; i++)
			ctr[i + 12] = (x >> ((3 - i) * 8)) & 0xFF;
	}
}

void sha_wait_idle()
{
	while(*REG_SHA_CNT & 1);
}

void sha(void* res, const void* src, uint32_t size, uint32_t mode)
{
	sha_wait_idle();
	*REG_SHA_CNT = mode | SHA_CNT_OUTPUT_ENDIAN | SHA_NORMAL_ROUND;
	
	const uint32_t* src32	= (const uint32_t*)src;
	int i;
	while(size >= 0x40)
	{
		sha_wait_idle();
		for(i = 0; i < 4; ++i)
		{
			*REG_SHA_INFIFO = *src32++;
			*REG_SHA_INFIFO = *src32++;
			*REG_SHA_INFIFO = *src32++;
			*REG_SHA_INFIFO = *src32++;
		}

		size -= 0x40;
	}
	
	sha_wait_idle();
	memcpy((void*)REG_SHA_INFIFO, src32, size);
	
	*REG_SHA_CNT = (*REG_SHA_CNT & ~SHA_NORMAL_ROUND) | SHA_FINAL_ROUND;
	
	while(*REG_SHA_CNT & SHA_FINAL_ROUND);
	sha_wait_idle();
	
	uint32_t hashSize = SHA_256_HASH_SIZE;
	if(mode == SHA_224_MODE)
		hashSize = SHA_224_HASH_SIZE;
	else if(mode == SHA_1_MODE)
		hashSize = SHA_1_HASH_SIZE;

	memcpy(res, (void*)REG_SHA_HASH, hashSize);
}

void rsa_wait_idle()
{
	while(*REG_RSA_CNT & 1);
}

void rsa_use_keyslot(uint32_t keyslot)
{
	*REG_RSA_CNT = (*REG_RSA_CNT & ~RSA_CNT_KEYSLOTS) | (keyslot << 4);
}

void rsa_setkey(uint32_t keyslot, const void* mod, const void* exp, uint32_t mode)
{
	rsa_wait_idle();
	*REG_RSA_CNT = (*REG_RSA_CNT & ~RSA_CNT_KEYSLOTS) | (keyslot << 4) | RSA_IO_BE | RSA_IO_NORMAL;
	
	uint32_t size = mode * 4;
	
	volatile uint32_t* keyslotCnt = REG_RSA_SLOT0 + (keyslot << 4);
	keyslotCnt[0] &= ~(RSA_SLOTCNT_KEY_SET | RSA_SLOTCNT_WPROTECT);
	keyslotCnt[1] = mode;
	
	memcpy((void*)REG_RSA_MOD_END - size, mod, size);
	
	if(exp == NULL)
	{
		size -= 4;
		while(size)
		{
			*REG_RSA_EXPFIFO = 0;
			size -= 4;
		}
		*REG_RSA_EXPFIFO = 0x01000100; // 0x00010001 byteswapped
	}
	else
	{
		const uint32_t* exp32 = (const uint32_t*)exp;
		while(size)
		{
			*REG_RSA_EXPFIFO = *exp32++;
			size -= 4;
		}
	}
}

int rsa_iskeyset(uint32_t keyslot)
{
	return *(REG_RSA_SLOT0 + (keyslot << 4)) & 1;
}

void rsa(void* dst, const void* src, uint32_t size)
{
	uint32_t keyslot = (*REG_RSA_CNT & RSA_CNT_KEYSLOTS) >> 4;
	if(rsa_iskeyset(keyslot) == 0)
		return;

	rsa_wait_idle();
	*REG_RSA_CNT |= RSA_IO_BE | RSA_IO_NORMAL;
	
	// Pad the message with zeroes so that it's a multiple of 8
	// and write the message with the end aligned with the register
	uint32_t padSize = ((size + 7) & ~7) - size;
	memset((void*)REG_RSA_TXT_END - (size + padSize), 0, padSize);
	memcpy((void*)REG_RSA_TXT_END - size, src, size);
	
	// Start
	*REG_RSA_CNT |= RSA_CNT_START;
	
	rsa_wait_idle();
	memcpy(dst, (void*)REG_RSA_TXT_END - size, size);
}

int rsa_verify(const void* data, uint32_t size, const void* sig, uint32_t mode)
{
	uint8_t dataHash[SHA_256_HASH_SIZE];
	sha(dataHash, data, size, SHA_256_MODE);
	
	uint8_t decSig[0x100]; // Way too big, need to request a work area
	
	uint32_t sigSize = mode * 4;
	rsa(decSig, sig, sigSize);
	
	return memcmp(dataHash, decSig + (sigSize - SHA_256_HASH_SIZE), SHA_256_HASH_SIZE) == 0;
}
