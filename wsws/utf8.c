/*
	[WSWS]  /utf8.c , 150406   (C) 2005-2015 MF
*/


#include <stdint.h>
#include <Windows.h>

#include "crypt.h"



/*
   Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
   See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
*/
int utf8_valid (const char *buffer, unsigned int buffer_size)
{
	static uint8_t map [ ] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3,
		0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
		0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1,
		1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,
		1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1,
		1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	};

	size_t  i;

	uint32_t  state = 0;
	uint32_t  point = 0;

	for(i = 0; i < buffer_size; i ++)
	{
		uint8_t  byte = (uint8_t) buffer [i];
		uint32_t type = map [byte];

		if(state)
			point = (byte & 0x3F) | (point << 6);
		else
			point = (0xFF >> type) & (byte);

		if(point > 0x1FFFFF)
			return 0;

		state = map [256 + state * 16 + type];
	}

	if(state == 0)
		return 1;
	else
		return 0;
}




int conv_utf8_to_unic (void *target, unsigned int *target_size, const void *source, unsigned int source_size)
{
	int  unicode_length;

	if((unicode_length = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH) source, (int) source_size, NULL, 0)) == 0)
		return 0;
	if(*target_size < ((unsigned int) unicode_length * sizeof(wchar_t)))
		return 0;

	MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, (LPCCH) source, (int) source_size, (LPWSTR) target, unicode_length);

	*(((wchar_t *) target) + unicode_length) = '\0';

	*target_size = (unsigned int) unicode_length;

	return 1;
}


int conv_unic_to_ansi (void *target, unsigned int *target_size, const void *source, unsigned int source_size)
{
	int  ansi_length;

	if((ansi_length = WideCharToMultiByte (CP_ACP, 0, (LPCWCH) source, (int) source_size, NULL, 0, NULL, NULL)) == 0)
		return 0;
	if(*target_size < (unsigned int) ansi_length)
		return 0;

	WideCharToMultiByte (CP_ACP, 0, (LPCWCH) source, (int) source_size, (char *) target, ansi_length, NULL, NULL);

	*(((char *) target) + ansi_length) = '\0';

	*target_size = (unsigned int) ansi_length;

	return 1;
}


void * conv_ansi_to_unic (unsigned int *target_size, const void *source, unsigned int source_size)
{
	int    unicode_length  = 0;
	void * unicode_buffer  = NULL;

	if((unicode_length = MultiByteToWideChar (CP_ACP, 0, (LPCCH) source, (int) source_size, NULL, 0)) == 0)
		return NULL;
	if((unicode_buffer = calloc (unicode_length + 1, sizeof(wchar_t))) == NULL)
		return NULL;

	MultiByteToWideChar (CP_ACP, 0, (LPCCH) source, (int) source_size, (LPWSTR) unicode_buffer, unicode_length);

	*target_size  = (unsigned int) unicode_length;

	return unicode_buffer;
}


void * conv_unic_to_utf8 (unsigned int *target_size, const void *source, unsigned int source_size)
{
	int    utf8_length = 0;
	void * utf8_buffer = NULL;

	if((utf8_length = WideCharToMultiByte (CP_UTF8, 0, (LPCWCH) source, source_size, NULL, 0, NULL, NULL)) == 0)
		return NULL;
	if((utf8_buffer = (char *) calloc (utf8_length + 1, sizeof(char))) == NULL)
		return NULL;

	WideCharToMultiByte (CP_UTF8, 0, (LPCWCH) source, source_size, (LPSTR) utf8_buffer, utf8_length, NULL, NULL);

	*target_size = (unsigned int) utf8_length;

	return utf8_buffer;
}


