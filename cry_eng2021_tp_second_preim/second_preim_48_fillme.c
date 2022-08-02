#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "xoshiro256starstar.h"
#include "hashtable.c"
#include <time.h>

#define ROTL24_16(x) ((((x) << 16) ^ ((x) >> 8)) & 0xFFFFFF)
#define ROTL24_3(x) ((((x) << 3) ^ ((x) >> 21)) & 0xFFFFFF)

#define ROTL24_8(x) ((((x) << 8) ^ ((x) >> 16)) & 0xFFFFFF)
#define ROTL24_21(x) ((((x) << 21) ^ ((x) >> 3)) & 0xFFFFFF)

#define IV 0x010203040506ULL 

/*
 * the 96-bit key is stored in four 24-bit chunks in the low bits of k[0]...k[3]
 * the 48-bit plaintext is stored in two 24-bit chunks in the low bits of p[0], p[1]
 * the 48-bit ciphertext is written similarly in c
 */
void speck48_96(const uint32_t k[4], const uint32_t p[2], uint32_t c[2])
{
	uint32_t rk[23];
	uint32_t ell[3] = {k[1], k[2], k[3]};

	rk[0] = k[0];

	c[0] = p[0];
	c[1] = p[1];

	/* full key schedule */
	for (unsigned i = 0; i < 22; i++)
	{
		uint32_t new_ell = ((ROTL24_16(ell[0]) + rk[i]) ^ i) & 0xFFFFFF; // addition (+) is done mod 2**24
		rk[i+1] = ROTL24_3(rk[i]) ^ new_ell;
		ell[0] = ell[1];
		ell[1] = ell[2];
		ell[2] = new_ell;
	}

	for (unsigned i = 0; i < 23; i++)
	{
		c[0] = ((ROTL24_16(c[0]) + c[1]) ^ rk[i])& 0xFFFFFF;
		c[1] = ROTL24_3(c[1]) ^ c[0];
	}

	return;
}

/* the inverse cipher */
void speck48_96_inv(const uint32_t k[4], const uint32_t c[2], uint32_t p[2])
{
	/* FILL ME */
	uint32_t rk[23];
	uint32_t ell[3] = {k[1], k[2], k[3]};

	rk[0] = k[0];

	p[0] = c[0];
	p[1] = c[1];

	/* full key schedule */
	for (unsigned i = 0; i < 22; i++)
	{
		uint32_t new_ell = ((ROTL24_16(ell[0]) + rk[i]) ^ i) & 0xFFFFFF; // addition (+) is done mod 2**24
		rk[i+1] = ROTL24_3(rk[i]) ^ new_ell;
		ell[0] = ell[1];
		ell[1] = ell[2];
		ell[2] = new_ell;
	}

	for (unsigned i = 0; i < 23; i++)
	{
		p[1] ^= p[0];
		p[1] = ROTL24_21(p[1]);
		p[0] ^= rk[22-i];
		p[0] = (p[0] - p[1])& 0xFFFFFF;
		p[0] = ROTL24_8(p[0]);
		
	}

	return;
}

/* The Davies-Meyer compression function based on speck48_96,
 * using an XOR feedforward
 * The input/output chaining value is given on the 48 low bits of a single 64-bit word,
 * whose 24 lower bits are set to the low half of the "plaintext"/"ciphertext" (p[0]/c[0])
 * and whose 24 higher bits are set to the high half (p[1]/c[1])
 */
uint64_t cs48_dm(const uint32_t m[4], const uint64_t h)
{
	/* FILL ME */
	uint32_t p[2];
	p[0] = h & 0xFFFFFF;
	p[1] = (h>>24) & 0xFFFFFF;
	uint64_t hash48 = p[1];
	hash48 = hash48 << 24;
	hash48 = hash48 & 0xFFFFFF000000;
	hash48 += p[0];

	uint32_t c[2];
	c[0] = 0x0;
	c[1] = 0x0;
	speck48_96(m, p, c);

	uint64_t hash = c[1];
	hash = hash << 24;
	hash = hash & 0xFFFFFF000000;
	hash += (c[0]&0xFFFFFF);

	hash ^= hash48;

	return hash;
}

/* assumes message length is fourlen * four blocks of 24 bits, each stored as the low bits of 32-bit words
 * fourlen is stored on 48 bits (as the 48 low bits of a 64-bit word)
 * when padding is include, simply adds one block (96 bits) of padding with fourlen and zeros on higher pos */
uint64_t hs48(const uint32_t *m, uint64_t fourlen, int padding, int verbose)
{
	uint64_t h = IV;
	const uint32_t *mp = m;

	for (uint64_t i = 0; i < fourlen; i++)
	{
		h = cs48_dm(mp, h);
		if (verbose)
			printf("@%lu : %06X %06X %06X %06X => %06lX\n", i, mp[0], mp[1], mp[2], mp[3], h);
		mp += 4;
	}
	if (padding)
	{
		uint32_t pad[4];
		pad[0] = fourlen & 0xFFFFFF;
		pad[1] = (fourlen >> 24) & 0xFFFFFF;
		pad[2] = 0;
		pad[3] = 0;
		h = cs48_dm(pad, h);
		if (verbose)
			printf("@%lu : %06X %06X %06X %06X => %06lX\n", fourlen, pad[0], pad[1], pad[2], pad[3], h);
	}

	return h;
}

/* Computes the unique fixed-point for cs48_dm for the message m */
uint64_t get_cs48_dm_fp(uint32_t m[4])
{
	/* FILL ME */
	uint32_t fixedPointGuessed[2];
	fixedPointGuessed[0] = 0x0;
	fixedPointGuessed[1] = 0x0;

	uint32_t cipher[2];
	cipher[0] = 0x0;
	cipher[1] = 0x0;

	speck48_96_inv(m, cipher, fixedPointGuessed);

	uint64_t result = fixedPointGuessed[1];
	result = result << 24;
	result = result & 0xFFFFFF000000;
	result += (fixedPointGuessed[0]&0xFFFFFF);

	return result;
}

/* Finds a two-block expandable message for hs48, using a fixed-point
 * That is, computes m1, m2 s.t. hs48_nopad(m1||m2) = hs48_nopad(m1||m2^*),
 * where hs48_nopad is hs48 with no padding 
*/
void find_exp_mess(uint32_t m1[4], uint32_t m2[4])
{
	int i;
	uint64_t random1, random2, res;
	//Implement Meet in the midle
	int SIZE = 9000000;
	struct DataItem** tabHash = malloc(sizeof(struct DataItem*) * SIZE); 
	//Set all pointer in our tab to NULL
	for(i = 0; i < SIZE; i++){
		tabHash[i] = NULL;
	}
	for(i = 0; i < 9000000; i++){
		random1 = xoshiro256starstar_random();
		random2 = xoshiro256starstar_random();
		m1[0] = random1 & 0xFFFFFF;
		m1[1] = random1>>32 & 0xFFFFFF;
		m1[2] = random2 & 0xFFFFFF;
		m1[3] = random2>>32 & 0xFFFFFF;
		res = cs48_dm(m1, IV);
		insertHashTable(res, m1, tabHash, SIZE);
	}
    while(true){
		random1 = xoshiro256starstar_random();
		random2 = xoshiro256starstar_random();
		m2[0] = random1 & 0xFFFFFF;
		m2[1] = random1>>32 & 0xFFFFFF;
		m2[2] = random2 & 0xFFFFFF;
		m2[3] = random2>>32 & 0xFFFFFF;
		res = get_cs48_dm_fp(m2);
		if(searchHashTable(res, m1, tabHash, SIZE)==1){
			break;
		}
	}
	freeHashTable(tabHash, SIZE);
	free(tabHash);
}

void attack(void)
{
	clock_t start = clock();

	uint32_t mess[1<<20];
	uint32_t test[4];
	uint64_t key = IV;
	int SIZE = 9000000;
	struct DataItem** hashArray = malloc(sizeof(struct DataItem*) * SIZE);
	int i;
	//Set all pointer in our tab to NULL
	for(i = 0; i < SIZE; i++){
		hashArray[i] = NULL;
	}
	for (i = 0; i < (1 << 20); i+=4)
	{
		mess[i + 0]	= i;
		mess[i + 1] = 0;
		mess[i + 2] = 0;
		mess[i + 3] = 0;

		test[0]	= i;
		test[1] = 0;
		test[2] = 0;
		test[3] = 0;
		key = cs48_dm(test, key);
		insertHashTable(key, test, hashArray, SIZE);
	}

	key = hs48(mess, (1<<20)/4, 1, 0);
	
	uint32_t *m1 = malloc(sizeof(uint32_t) * 4);
	uint32_t *m2 = malloc(sizeof(uint32_t) * 4);
	find_exp_mess(m1, m2);
	uint64_t fp = cs48_dm(m1, IV);
	printf("\nExpendable message found \n%x\n%x\n%x\n%x\n\n%x\n%x\n%x\n%x\n", m1[0], m1[1], m1[2], m1[3], m2[0], m2[1], m2[2], m2[3]);
	
	//Search message
	uint64_t random1, random2, res;
	uint32_t *m3 = malloc(sizeof(uint32_t) * 4);
	uint32_t *m4 = malloc(sizeof(uint32_t) * 4);
	int numberOfTry = 0;
    while(true){
		random1 = xoshiro256starstar_random();
		random2 = xoshiro256starstar_random();
		m3[0] = random1 & 0xFFFFFF;
		m3[1] = random1>>24 & 0xFFFFFF;
		m3[2] = random2 & 0xFFFFFF;
		m3[3] = random2>>24 & 0xFFFFFF;
		res = cs48_dm(m3, fp);
		if(searchHashTable(res, m4, hashArray, SIZE)==1){
			break;
		}
		numberOfTry++;
	}
	mess[0]	= m1[0];
	mess[1] = m1[1];
	mess[2] = m1[2];
	mess[3] = m1[3];
	for (i = 4; i < (1 << 20); i+=4)
	{
		if(i < m4[0]){
			mess[i + 0]	= m2[0];
			mess[i + 1] = m2[1];
			mess[i + 2] = m2[2];
			mess[i + 3] = m2[3];
		} else  if (i == m4[0]) {
			mess[i + 0]	= m3[0];
			mess[i + 1] = m3[1];
			mess[i + 2] = m3[2];
			mess[i + 3] = m3[3];
		} else {
			mess[i + 0]	= i;
			mess[i + 1] = 0;
			mess[i + 2] = 0;
			mess[i + 3] = 0;
		}
	}
	printf("\nCollision find at step \n%d\nIn %d try\n", m4[0], numberOfTry);
	printf("Key supposed \n%lx\n", key);
	key = hs48(mess, (1<<20)/4, 1, 0);
	printf("Key found \n%lx\nFor the collision message \n%x\n%x\n%x\n%x\n", key, m3[0], m3[1], m3[2], m3[3]);
	freeHashTable(hashArray, SIZE);
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	printf("\nIn %f seconds\n", seconds);
}

/**
 * Test our implementation of find_exp_mess.
 * Return 1 if our implementation is correct, 0 otherwise
*/
int test_em() {
	uint32_t *m1 = malloc(sizeof(uint32_t) * 4);
	uint32_t *m2 = malloc(sizeof(uint32_t) * 4);

	find_exp_mess(m1, m2);
	uint64_t h = cs48_dm(m1, IV);
	uint64_t h1 = get_cs48_dm_fp(m2);
	int result = 0;
	if(h == h1){
		result = 1;
	}

	free(m1);
	free(m2);
	//freeHashTable();
	return result;
}

/**
 * Test our implementation of speck48_96 with value get in 
 * "The Simon and Speck families of lightwweight Block Ciphers"
 * Return 1 if our implementation is correct, 0 otherwise
*/
int test_sp48(){
	uint32_t *supposedCipherText = malloc(sizeof(uint32_t) * 2);
    uint32_t *plainText = malloc(sizeof(uint32_t) * 2);
    uint32_t *key = malloc(sizeof(uint32_t) * 4);
    key[0] = 0x020100;
    key[1] = 0x0a0908;
    key[2] = 0x121110;
    key[3] = 0x1a1918;
    plainText[0] = 0x6d2073;
    plainText[1] = 0x696874;
	uint32_t cipher[2];

    speck48_96(key, plainText, cipher);
	supposedCipherText[0] = 0x735e10;
	supposedCipherText[1] = 0xb6445d;
    int i, result = 1;
    for(i = 0; i < 2; i++){
		if(supposedCipherText[i] != cipher[i]){
			result = 0;
		}
    }
    
    free(key);
    free(plainText);
    free(supposedCipherText);
    return result;
}

/**
 * Test our implementation of speck48_96_inv with value get in 
 * "The Simon and Speck families of lightwweight Block Ciphers"
 * Return 1 if our implementation is correct, 0 otherwise
*/
int test_sp48_inv(){
	uint32_t *cipherText = malloc(sizeof(uint32_t) * 2);
    uint32_t *supposedPlainText = malloc(sizeof(uint32_t) * 2);
    uint32_t *key = malloc(sizeof(uint32_t) * 4);
    key[0] = 0x020100;
    key[1] = 0x0a0908;
    key[2] = 0x121110;
    key[3] = 0x1a1918;
    supposedPlainText[0] = 0x696874;
	supposedPlainText[1] = 0x6d2073;
	uint32_t plainText[2];

    int i, result = 1;

	speck48_96(key, supposedPlainText, cipherText);
	speck48_96_inv(key, cipherText, plainText);
	for(i = 0; i < 2; i++){
		if(supposedPlainText[i] != plainText[i]){
			result = 0;
		}
    }
    free(key);
    free(cipherText);
    free(supposedPlainText);
    return result;
}

/**
 * Take 2 24 bits number and store them in uint64_t
 * the first number will be at the 24 lowest bits and the 
 * second number will be store to the 24+1 bits to the 48 bits.
 * returne the uint64_t that store them
*/
uint64_t fill_uint64_with_2_uint32_t(uint32_t first_number, uint32_t second_number){
	uint64_t h;
	h = second_number;
	h = h << 24;
	h = h & 0xFFFFFF000000;
	h += first_number;
	return h;
}

/**
 * Test our implementation of cs48_dm with the all-zero input
 * Return 1 if our implementation is correct, 0 otherwise
*/
int test_cs48_dm(){
	uint32_t *text = malloc(sizeof(uint32_t) * 2);
    uint32_t *supposedHash = malloc(sizeof(uint32_t) * 2);
    uint32_t *key = malloc(sizeof(uint32_t) * 4);
    key[0] = 0x00000000;
    key[1] = 0x00000000;
    key[2] = 0x00000000;
    key[3] = 0x00000000;

	text[0] = 0x00000000;
	text[1] = 0x00000000;

	supposedHash[0] = 0x6EB248;
	supposedHash[1] = 0x7FDD5A;

	//uint64_t h = fill_uint64_with_2_uint32_t(0x696874, 0x6d2073);
	uint64_t h = fill_uint64_with_2_uint32_t(text[0], text[1]);
    uint64_t resultHash = cs48_dm(key, h);
	uint32_t resultHash32[2];
	resultHash32[0] = resultHash & 0xFFFFFF;
	resultHash32[1] = (resultHash>>24) & 0xFFFFFF;
    int i, result = 1;
    for(i = 0; i < 2; i++){
		if(supposedHash[i] != resultHash32[i]){
			result = 0;
		}
    }
    
    free(key);
    free(text);
    free(supposedHash);
    return result;
}

/**
 * Test if the function cs48_dm_fp return the fixed point
 * 1 If it's true, 0 otherwise
*/
int test_cs48_dm_fp(){
	uint32_t *messageM = malloc(sizeof(uint32_t) * 4);
    messageM[0] = 0x00000000;
    messageM[1] = 0x00000000;
    messageM[2] = 0x00000000;
    messageM[3] = 0x00000000;
    
    uint64_t resultFP = get_cs48_dm_fp(messageM);
	uint64_t resultHash = cs48_dm(messageM, resultFP);
    int result = 1;
    if(resultHash != resultFP) {
		result = 0;
	}
    
    free(messageM);
	return result;
}

int main()
{
#ifdef RUNTESTS
	int speck48_96_is_correct = test_sp48();
	if(speck48_96_is_correct == 1){
		printf("Our function speck48_96 is correctly implemented !\n");
	} else {
		printf("We have an error in the function speck48_96\n");
		return 0;
	}

	int speck48_96_inv_is_correct = test_sp48_inv();
	if(speck48_96_inv_is_correct == 1){
		printf("Our function speck48_96_inv is correctly implemented !\n");
	} else {
		printf("We have an error in the function speck48_96_inv\n");
		return 0;
	}

	int scs48_dm_is_correct = test_cs48_dm();
	if(scs48_dm_is_correct == 1){
		printf("Our function cs48_dm is correctly implemented !\n");
	} else {
		printf("We have an error in the function cs48_dm\n");
		return 0;
	}

	int scs48_dm_fp_is_correct = test_cs48_dm_fp();
	if(scs48_dm_fp_is_correct == 1){
		printf("Our function cs48_dm_fp is correctly implemented !\n");
	} else {
		printf("We have an error in the function cs48_dm_fp\n");
		return 0;
	}
	
	int em_is_correct = test_em();
	if(em_is_correct == 1){
		printf("Our function find_exp_mess is correctly implemented !\n");
	} else {
		printf("We have an error in the function find_exp_mess\n");
		return 0;
	}
#endif
	attack();

	return 0;
}
