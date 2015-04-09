/*
	[WSWS]  /client.c , 150407   (C) 2005-2015 MF
*/


#include <stdlib.h>
#include <string.h>

#include "wsws.h"
#include "util.h"
#include "crypt.h"



static int ws_send (const ws_client_t *client, const ws_message_t *message)
{
	uint8_t header_data [10] = {0};
	size_t  header_size      =  2 ;

	if(message->binary)
		header_data [0] = 0x82;
	else
		header_data [0] = 0x81;

	if(message->size < 126)
		header_data [1] = (uint8_t) message->size;
	else if(message->size < 0x10000)
	{
		header_size += 2;

		header_data [1] = 126;
		header_data [2] = (message->size >> 8) & 0xFF;
		header_data [3] = (message->size     ) & 0xFF;
	}
	else
	{
		header_size += 8;

		header_data [1] = 127;
		header_data [6] = (message->size >> 24) & 0xFF;
		header_data [7] = (message->size >> 16) & 0xFF;
		header_data [8] = (message->size >>  8) & 0xFF;
		header_data [9] = (message->size      ) & 0xFF;
	}

	if(ws_util_send (client->reserved.socket, (char *) header_data, header_size) == 0)
		return 0;

	if(message->size > 0)
	{
		if(ws_util_send (client->reserved.socket, message->text, message->size) == 0)
			return 0;
	}

	((ws_client_t *) client)->size_sent += header_size + message->size;

	return 1;
}


int ws_send_binary (const ws_client_t *client, const void *buffer, size_t buffer_size)
{
	ws_message_t message;

	message.binary = 1;
	message.data   = buffer;
	message.size   = buffer_size;

	return ws_send (client, & message);
}


int ws_send_text (const ws_client_t *client, const void *text, int text_length, int ansi)
{
	ws_message_t message;

	size_t  buffer_size = 0;

	if(text_length < 0)
	{
		if(ansi)
			buffer_size = strlen ((char *   ) text);
		else
			buffer_size = wcslen ((wchar_t *) text);
	}
	else
		buffer_size = (size_t) text_length;

	if(buffer_size == 0)
	{
		message.binary = 0;
		message.size   = 0;

		return ws_send (client, & message);
	}
	else
	{
		int  result  = 0;

		unsigned int  buffer_middle_size  = 0;
		unsigned int  buffer_target_size  = 0;

		void *  buffer_middle_data  = NULL;
		void *  buffer_target_data  = NULL;

		if(ansi)
		{
			if((buffer_middle_data = conv_ansi_to_unic (& buffer_middle_size, text, text_length)) == NULL)
				goto end;

			buffer_target_data = conv_unic_to_utf8 (& buffer_target_size, buffer_middle_data, buffer_middle_size);
		}
		else
			buffer_target_data = conv_unic_to_utf8 (& buffer_target_size, text, text_length);

		if(buffer_target_data == NULL)
			goto end;

		message.binary = 0;
		message.size   = (size_t) buffer_target_size;
		message.data   = buffer_target_data;

		result = ws_send (client, & message);

end:
		if(buffer_middle_data)
			free (buffer_middle_data);
		if(buffer_target_data)
			free (buffer_target_data);

		return result;
	}
}




void ws_disconnect (const ws_client_t *client)
{
	ws_util_handshake_close (client->reserved.socket, WS_UTIL_CLOSE_OK);
	ws_util_closed_set      (client->reserved.closed);
}


