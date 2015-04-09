/*
	[WSWS]  /base64.c , 150407   (C) 2005-2015 MF
*/


#include <string.h>

#include "crypt.h"



static void base64_encode (unsigned char *target, const unsigned char *source)
{
	static const char * map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	int i;
	int value;

	value  = source [0];
	value *= 256;
	value += source [1];
	value *= 256;
	value += source [2];

	for(i = 0; i < 4; i ++)
	{
		target [3 - i] = map [value % 64];

		value /= 64;
	}
}


void base64 (char *target, const char *source, unsigned int source_size)
{
	while(source_size >= 3)
	{
		base64_encode ((unsigned char *) target, (const unsigned char *) source);

		source += 3;
		target += 4;

		source_size -= 3;
	}

	if(source_size > 0)
	{
		char temp [3] = {0};

		memcpy (temp, source, source_size);

		base64_encode ((unsigned char *) target, (const unsigned char *) temp);

		target [2] = (source_size == 1) ? '=' : target [2];
		target [3] = '=';

		target += 4;
	}

	* target = '\0';
}


