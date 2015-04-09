/*
	[WSWS]  /sha1.c , 150407   (C) 2005-2015 MF
*/


#include "crypt.h"



typedef struct
{
	int  index;

	unsigned int  digest [ 5];
	unsigned char block  [64];
}
ws_sha1_context_t;




static unsigned int sha1_circular_shift (unsigned int bits, unsigned int word)
{
	return (((word) << (bits)) | ((word) >> (32 - (bits))));
}


static void sha1_message_block (ws_sha1_context_t *context)
{
	static unsigned int K [ ] =  { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

	int  i;
	unsigned int  t, A, B, C, D, E, W [80];

	for(i = 0; i < 16; i ++)
	{
		W [i]  = ((unsigned int) context->block [i * 4    ]) << 24;
		W [i] |= ((unsigned int) context->block [i * 4 + 1]) << 16;
		W [i] |= ((unsigned int) context->block [i * 4 + 2]) <<  8;
		W [i] |= ((unsigned int) context->block [i * 4 + 3])      ;
	}

	for(i = 16; i < 80; i ++)
		W [i] = sha1_circular_shift (1, W [i - 3] ^ W [i - 8] ^ W [i - 14] ^ W [i - 16]);

	A  = context->digest [0];
	B  = context->digest [1];
	C  = context->digest [2];
	D  = context->digest [3];
	E  = context->digest [4];

	for(i = 0; i < 20; i ++)
	{
		t  = sha1_circular_shift (5, A) + ((B & C) | ((~ B) & D)) + E + W [i] + K [0];
		E  = D;
		D  = C;
		C  = sha1_circular_shift (30, B);
		B  = A;
		A  = t;
	}

	for(i = 20; i < 40; i ++)
	{
		t  = sha1_circular_shift (5, A) + (B ^ C ^ D) + E + W [i] + K [1];
		E  = D;
		D  = C;
		C  = sha1_circular_shift (30, B);
		B  = A;
		A  = t;
	}

	for(i = 40; i < 60; i ++)
	{
		t  = sha1_circular_shift (5, A) + ((B & C) | (B & D) | (C & D)) + E + W [i] + K [2];
		E  = D;
		D  = C;
		C  = sha1_circular_shift (30, B);
		B  = A;
		A  = t;
	}

	for(i = 60; i < 80; i ++)
	{
		t  = sha1_circular_shift (5, A) + (B ^ C ^ D) + E + W [i] + K [3];
		E  = D;
		D  = C;
		C  = sha1_circular_shift (30, B);
		B  = A;
		A  = t;
	}

	context->index  = 0;

	context->digest [0] += A;
	context->digest [1] += B;
	context->digest [2] += C;
	context->digest [3] += D;
	context->digest [4] += E;
}


void sha1 (char *target, const char *source, unsigned int source_size)
{
	unsigned int  i;

	ws_sha1_context_t context;

	char * data    = (char *) & context.digest;

	context.index  = 0;
	
	context.digest [0] = 0x67452301;
	context.digest [1] = 0xEFCDAB89;
	context.digest [2] = 0x98BADCFE;
	context.digest [3] = 0x10325476;
	context.digest [4] = 0xC3D2E1F0;

	for(i = 0; i < source_size; i ++)
	{
		context.block [context.index ++] = ((unsigned char ) * (source ++)) & 0xFF;

		if(context.index == 64)
			sha1_message_block (& context);
	}

	context.block [context.index ++] = 0x80;

	if(context.index > 55)
	{
		while(context.index < 64)
			context.block [context.index ++] = 0;

		sha1_message_block (& context);
	}

	while(context.index < 56)
		context.block [context.index ++] = 0;

	context.block [56] = 0;
	context.block [57] = 0;
	context.block [58] = 0;
	context.block [59] = 0;
	context.block [60] = (source_size  >> 21) & 0xFF;
	context.block [61] = (source_size  >> 13) & 0xFF;
	context.block [62] = (source_size  >>  5) & 0xFF;
	context.block [63] = (source_size  <<  3) & 0xFF;

	sha1_message_block (& context);

	target [ 0] = data [ 3];  target [ 1] = data [ 2];  target [ 2] = data [ 1];  target [ 3] = data [ 0];
	target [ 4] = data [ 7];  target [ 5] = data [ 6];  target [ 6] = data [ 5];  target [ 7] = data [ 4];
	target [ 8] = data [11];  target [ 9] = data [10];  target [10] = data [ 9];  target [11] = data [ 8];
	target [12] = data [15];  target [13] = data [14];  target [14] = data [13];  target [15] = data [12];
	target [16] = data [19];  target [17] = data [18];  target [18] = data [17];  target [19] = data [16];
}


