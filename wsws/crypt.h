/*
	[WSWS]  /crypt.h , 150407   (C) 2005-2015 MF
*/


#ifndef _WSWS_CRYPT_H_
#define _WSWS_CRYPT_H_



#ifdef __cplusplus
	extern "C" {
#endif



void sha1   (char *target, const char *source, unsigned int source_size);
void base64 (char *target, const char *source, unsigned int source_size);



int    utf8_valid (const char *buffer, unsigned int buffer_size);

int    conv_utf8_to_unic (void *target, unsigned int *target_size, const void *source, unsigned int source_size);
int    conv_unic_to_ansi (void *target, unsigned int *target_size, const void *source, unsigned int source_size);

void * conv_ansi_to_unic (unsigned int *target_size, const void *source, unsigned int source_size);
void * conv_unic_to_utf8 (unsigned int *target_size, const void *source, unsigned int source_size);



#ifdef __cplusplus
	}
#endif


#endif   /* _WSWS_CRYPT_H_ */