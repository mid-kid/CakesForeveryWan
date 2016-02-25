// From http://github.com/b1l1s/ctr

#pragma once

#include <stdint.h>
#include "headers.h"

/**************************AES****************************/
#define REG_AESCNT				((volatile uint32_t*)0x10009000)
#define REG_AESBLKCNT			((volatile uint32_t*)0x10009004)
#define REG_AESWRFIFO			((volatile uint32_t*)0x10009008)
#define REG_AESRDFIFO			((volatile uint32_t*)0x1000900C)
#define REG_AESKEYSEL			((volatile uint8_t *)0x10009010)
#define REG_AESKEYCNT			((volatile uint8_t *)0x10009011)
#define REG_AESCTR				((volatile uint32_t*)0x10009020)

#define REG_AESKEYFIFO			((volatile uint32_t*)0x10009100)
#define REG_AESKEYXFIFO			((volatile uint32_t*)0x10009104)
#define REG_AESKEYYFIFO			((volatile uint32_t*)0x10009108)

#define AES_CCM_DECRYPT_MODE	(0u << 27)
#define AES_CCM_ENCRYPT_MODE	(1u << 27)
#define AES_CTR_MODE			(2u << 27)
#define AES_CTR_MODE			(2u << 27)
#define AES_CBC_DECRYPT_MODE	(4u << 27)
#define AES_CBC_ENCRYPT_MODE	(5u << 27)
#define AES_ECB_DECRYPT_MODE	(6u << 27)
#define AES_ECB_ENCRYPT_MODE	(7u << 27)
#define AES_ALL_MODES			(7u << 27)

#define AES_CNT_START			0x80000000
#define AES_CNT_INPUT_ORDER		0x02000000
#define AES_CNT_OUTPUT_ORDER	0x01000000
#define AES_CNT_INPUT_ENDIAN	0x00800000
#define AES_CNT_OUTPUT_ENDIAN	0x00400000
#define AES_CNT_FLUSH_READ		0x00000800
#define AES_CNT_FLUSH_WRITE		0x00000400

#define AES_INPUT_BE			(AES_CNT_INPUT_ENDIAN)
#define AES_INPUT_LE			0
#define AES_INPUT_NORMAL		(AES_CNT_INPUT_ORDER)
#define AES_INPUT_REVERSED		0

#define AES_TEMP_KEYSLOT		0x11

#define AES_BLOCK_SIZE			0x10

#define AES_KEYCNT_WRITE		(1 << 0x7)
#define AES_KEYNORMAL			0
#define AES_KEYX				1
#define AES_KEYY				2

/**************************SHA****************************/
#define REG_SHA_CNT				((volatile uint32_t*)0x1000A000)
#define REG_SHA_BLKCNT			((volatile uint32_t*)0x1000A004)
#define REG_SHA_HASH			((volatile uint32_t*)0x1000A040)
#define REG_SHA_INFIFO			((volatile uint32_t*)0x1000A080)

#define SHA_CNT_STATE			0x00000003
#define SHA_CNT_UNK2			0x00000004
#define SHA_CNT_OUTPUT_ENDIAN	0x00000008
#define SHA_CNT_MODE			0x00000030
#define SHA_CNT_ENABLE			0x00010000
#define SHA_CNT_ACTIVE			0x00020000

#define SHA_HASH_READY			0x00000000
#define SHA_NORMAL_ROUND		0x00000001
#define SHA_FINAL_ROUND			0x00000002

#define SHA_OUTPUT_BE			SHA_CNT_OUTPUT_ENDIAN
#define SHA_OUTPUT_LE			0

#define SHA_256_MODE			0
#define SHA_224_MODE			0x00000010
#define SHA_1_MODE				0x00000020

#define SHA_256_HASH_SIZE		(256 / 8)
#define SHA_224_HASH_SIZE		(224 / 8)
#define SHA_1_HASH_SIZE			(160 / 8)

/**************************RSA****************************/
#define REG_RSA_CNT				((volatile uint32_t*)0x1000B000)
#define REG_RSA_SLOT0			((volatile uint32_t*)0x1000B100)
#define REG_RSA_SLOT1			((volatile uint32_t*)0x1000B110)
#define REG_RSA_SLOT2			((volatile uint32_t*)0x1000B120)
#define REG_RSA_SLOT3			((volatile uint32_t*)0x1000B130)
#define REG_RSA_EXPFIFO			((volatile uint32_t*)0x1000B200)
#define REG_RSA_MOD_END			((volatile uint32_t*)0x1000B500)
#define REG_RSA_TXT_END			((volatile uint32_t*)0x1000B900)

#define RSA_CNT_START			0x00000001
#define RSA_CNT_KEYSLOTS		0x000000F0
#define RSA_CNT_IO_ENDIAN		0x00000100
#define RSA_CNT_IO_ORDER		0x00000200

#define RSA_SLOTCNT_KEY_SET		0x00000001
#define RSA_SLOTCNT_WPROTECT	0x00000002 // Write protect

#define RSA_IO_BE				RSA_CNT_IO_ENDIAN
#define RSA_IO_LE				0
#define RSA_IO_NORMAL			RSA_CNT_IO_ORDER
#define RSA_IO_REVERSED			0

#define RSA_TEMP_KEYSLOT		0

#define RSA_1024_MODE			0x20
#define RSA_2048_MODE			0x40

void aes_setkey(uint8_t keyslot, const void* key, uint32_t keyType, uint32_t mode);
void aes_use_keyslot(uint8_t keyslot);

void aes(void* dst, const void* src, uint32_t blockCount, void* iv, uint32_t mode, uint32_t ivMode);

void aes_setiv(const void* iv, uint32_t mode);
void aes_advctr(void* ctr, uint32_t val, uint32_t mode);
void aes_change_ctrmode(void* ctr, uint32_t fromMode, uint32_t toMode);

void aes_batch(void* dst, const void* src, uint32_t blockCount);

void sha(void* res, const void* src, uint32_t size, uint32_t mode);

void rsa_setkey(uint32_t keyslot, const void* mod, const void* exp, uint32_t mode);
void rsa_use_keyslot(uint32_t keyslot);

int rsa_verify(const void* data, uint32_t size, const void* sig, uint32_t mode);

typedef enum
{
	NCCHTYPE_EXHEADER = 1,
	NCCHTYPE_EXEFS = 2,
	NCCHTYPE_ROMFS = 3,
} ctr_ncchtypes;

void ncch_getctr(const ncch_h* ncch, uint8_t* ctr, uint8_t type);
