/*
	[WSWS]  /util.c , 150406   (C) 2005-2015 MF
*/


#include <stdio.h>
#include <string.h>
#include <WinSock2.h>
#include <Windows.h>

#include "wsws.h"
#include "util.h"
#include "crypt.h"




ws_util_frame_status_e ws_util_frame (ws_client_t *client, ws_message_t *message, ws_util_close_e *close, char *buffer, size_t buffer_size, size_t *buffer_want, size_t *buffer_used)
{
	ws_context_t * context = (ws_context_t *) client->reserved.context;
	uint8_t *      data    = (uint8_t *     ) buffer;

	size_t  frame_size         = 6;
	size_t  frame_size_header  = 6;
	size_t  frame_size_payload = 0;
	
	uint8_t   value_fin     = 0;
	uint8_t   value_opcode  = 0;
	uint32_t  value_mask    = 0;

	if(frame_size > buffer_size)
		goto end_more;

	/* check RSV1/RSV2/RSV3 */
	if(data [0] & 0x70)
		goto end_close_error;
	/* check MASKED */
	if((data [1] & 0x80) == 0)
		goto end_close_error;

	/* get PAYLOAD_SIZE */
	switch(frame_size_payload = (size_t) (data [1] & 0x7F))
	{
		case 126:
			frame_size        += 2;
			frame_size_header += 2;

			if(buffer_size < 8)
				goto end_more;
			else
				frame_size_payload = (size_t) ntohs (*((uint16_t *) (data + 2)));

			break;

		case 127:
			frame_size        += 8;
			frame_size_header += 8;

			if(buffer_size < 14)
				goto end_more;
			else
			{
				uint64_t size_frame_payload_64 = *((uint64_t *) (data + 2));

				size_frame_payload_64 = ((((uint64_t) ntohl ((uint32_t) size_frame_payload_64)) << 32) & 0xFFFFFFFF00000000ULL) | (uint64_t) ntohl ((uint32_t) (size_frame_payload_64 >> 32));

				if(size_frame_payload_64 & 0xFFFFFFFF00000000ULL)
					goto end_close_toolarge;

				frame_size_payload = (size_t) size_frame_payload_64;
			}

			break;
	}

	value_fin     = data [0] & 0x80;
	value_opcode  = data [0] & 0x0F;
	value_mask    = *((uint32_t *) (data + frame_size_header - 4));

	frame_size   += frame_size_payload;

	/* check FRAME_SIZE */
	if(frame_size > context->max_size_of_frame)
		goto end_close_toolarge;
	if(frame_size > buffer_size)
		goto end_more;

	/* check OPCODE */
	switch(value_opcode)
	{
		/* continuation frame */
		case 0x00:
			if(message->binary == -1)
				goto end_close_error;
			else
				break;

		/* text frame */
		case 0x01:
		/* binary frame */
		case 0x02:
			if(message->binary == -1)
				break;
			else
				goto end_close_error;

		/* connection close */
		case 0x08:
		/* ping */
		case 0x09:
		/* pong */
		case 0x0A:
			if(value_fin == 0)
				goto end_close_error;
			if(frame_size_payload > 125)
				goto end_close_error;

			break;

		default:
			goto end_close_error;
	}

	/* response OPCODE */
	switch(value_opcode)
	{
		/* continuation frame */
		case 0x00:
		/* text frame */
		case 0x01:
		/* binary frame */
		case 0x02:
			if(message->binary == -1)
			{
				if(value_opcode == 0x01)
					message->binary = 0;
				else
					message->binary = 1;
			}

			if((message->size + (size_t) frame_size_payload) > context->max_size_of_data)
				goto end_close_toolarge;

			ws_util_unmask ((char *) (message->text + message->size), (char *) (data + (size_t) frame_size_header), (size_t) frame_size_payload, value_mask);

			message->size += (size_t) frame_size_payload;

			break;

		/* connection close */
		case 0x08:
			if(frame_size_payload == 2)
			{
				uint16_t close_code = *((uint16_t *) (data + frame_size_header));

				ws_util_unmask ((char *) & close_code, (char *) & close_code, sizeof(uint16_t), value_mask);

				close_code = ntohs (close_code);

				switch(close_code)
				{
					case 1000:  case 1001:  case 1002:  case 1003:  case 1007:
					case 1008:  case 1009:  case 1010:  case 1011:
						break;

					default:
						if(close_code < 3000)
							goto end_close_error;
						if(close_code > 4999)
							goto end_close_error;
				}

				break;
			}
			else if(frame_size_payload == 0)
				break;
			else if(frame_size_payload == 1)
				goto end_close_error;
			else
				goto end_close_null;

		/* ping */
		case 0x09:
			ws_util_pong (client->reserved.socket, data, (size_t) frame_size_payload);

			break;
	}

	/* detect return code */
	switch(value_opcode)
	{
		/* continuation frame */
		case 0x00:
		/* text frame */
		case 0x01:
		/* binary frame */
		case 0x02:
			if(value_fin)
				goto end_ok;
			else
				goto end_continue;

		/* connection close */
		case 0x08:
			goto end_close_ok;

		/* ping */
		case 0x09:
		/* pong */
		case 0x0A:
			goto end_continue;
	}

end_ok:
	* buffer_used = frame_size;		return WS_UTIL_FRAME_STATUS_OK;
end_more:	
	* buffer_want = frame_size;		return WS_UTIL_FRAME_STATUS_MORE;
end_continue:	
	* buffer_used = frame_size;		return WS_UTIL_FRAME_STATUS_CONTINUE;
end_break:
									return WS_UTIL_FRAME_STATUS_BREAK;

end_close_null:			*close = WS_UTIL_CLOSE_NULL;		goto end_break;
end_close_ok:			*close = WS_UTIL_CLOSE_OK;			goto end_break;
end_close_error:		*close = WS_UTIL_CLOSE_ERROR;		goto end_break;
end_close_toolarge:		*close = WS_UTIL_CLOSE_TOOLARGE;	goto end_break;
}




void ws_util_unmask (char *target, char *source, size_t size, uint32_t mask)
{
	size_t    i;
	uint8_t * mask_buffer = (uint8_t *) & mask;

	for(i = 0; i < size; i ++)
		*(target ++) = *(source ++) ^ mask_buffer [i & 0x03];
}




int ws_util_closed_get (void *closed)
{
	if(WaitForSingleObject ((HANDLE) closed, 0) == WAIT_OBJECT_0)
		return 1;
	else
		return 0;
}


void ws_util_closed_set (void *closed)
{
	SetEvent ((HANDLE) closed);
}




int ws_util_send (void *socket, const char *buffer, size_t buffer_size)
{
	while(buffer_size > 0)
	{
		int buffer_send = send ((SOCKET) socket, buffer, (int) buffer_size, 0);

		if(buffer_send == SOCKET_ERROR)
			return 0;

		buffer      += buffer_send;
		buffer_size -= buffer_send;
	}

	return 1;
}


size_t ws_util_recv (void *socket, char *buffer, size_t buffer_size, int all)
{
	int  buffer_size_read;

	int  flags = all ? MSG_WAITALL : 0;

	if(buffer_size == 0)
		return 0;
	if((buffer_size_read = recv ((SOCKET) socket, buffer, (int) buffer_size, flags)) <= 0)
		return 0;

	return (size_t) buffer_size_read;
}


void ws_util_pong (void *socket, uint8_t *ping, size_t payload_size)
{
	uint32_t mask = *((uint32_t *) (ping + 2));

	if(payload_size)
		ws_util_unmask ((char *) (ping + 2), (char *) (ping + 6), payload_size, mask);

	ping [0]  = 0x8A;
	ping [1] &= 0x7F;

	ws_util_send (socket, (char *) ping, payload_size + 2);
}




int ws_util_handshake_open (void *socket, char *buffer, size_t buffer_size)
{
	char   key_sha1   [ 24];
	char   key_base64 [128];

	size_t buffer_size_used = 0;
	size_t buffer_size_free = buffer_size - 1;

	memset (buffer, 0, buffer_size);

	while(buffer_size_free > 0)
	{
		size_t buffer_size_read = ws_util_recv (socket, buffer + buffer_size_used, buffer_size_free, 0);

		if(buffer_size_read == 0)
			break;

		if(strstr (buffer, "\r\n\r\n"))
			goto found;
		if(strstr (buffer, "\n\n"))
			goto found;

		buffer_size_used += buffer_size_read;
		buffer_size_free -= buffer_size_read;
	}

	return 0;

found:
	{
		char   key [64];

		char * key_from = NULL;
		char * key_to   = NULL;

		if((key_from = strstr (buffer, "Sec-WebSocket-Key: ")) == NULL)
			return 0;
		if((key_to   = strchr (key_from, '\n')) == NULL)
			return 0;

		if(*(key_to - 1) == '\r')
			-- key_to;

		if((key_to - key_from) != 43)
			return 0;

		memcpy (key,      key_from + 19,                          24);
		memcpy (key + 24, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);

		sha1   (key_sha1,   key,      60);
		base64 (key_base64, key_sha1, 20);
	}

	{
		char response [256];

		sprintf_s (response, sizeof response,
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: %s\r\n"
			"\r\n",
			key_base64
		);

		return ws_util_send (socket, response, strlen (response));
	}
}


void ws_util_handshake_close (void *socket, ws_util_close_e code)
{
	size_t buffer_size = 2;
	char   buffer [4]  = { 0x88, 0x00 };

	if(code != WS_UTIL_CLOSE_NULL)
	{
		buffer_size = 4;

		buffer [1] = 0x02;

		*((uint16_t *) (buffer + 2)) = htons ((uint16_t) code);
	}

	ws_util_send (socket, buffer, buffer_size);
}


