/*
	[WSWS]  /server.c , 150406   (C) 2005-2015 MF
*/


#include <stdio.h>
#include <process.h>
#include <Windows.h>

#include "wsws.h"
#include "util.h"
#include "crypt.h"




static void __cdecl ws_run_client_thread (ws_client_t *client)
{
	ws_context_t * context = (ws_context_t *) client->reserved.context;
	SOCKET         socket  = (SOCKET        ) client->reserved.socket;

	char *  buffer_frame       = NULL;
	char *  buffer_data_source = NULL;
	char *  buffer_data_target = NULL;
	char *  buffer_data_middle = NULL;

	ws_util_close_e close_code = WS_UTIL_CLOSE_OK;

	/* initialize ws_client_t */
	{
		struct sockaddr_in address;

		int address_length = sizeof address;

		if(getpeername (socket, (struct sockaddr *) & address, & address_length) != 0)
			goto end_0;

		sprintf_s (client->address, 24, "%u.%u.%u.%u",
			(address.sin_addr.S_un.S_addr      ) & 0xFF,
			(address.sin_addr.S_un.S_addr >>  8) & 0xFF,
			(address.sin_addr.S_un.S_addr >> 16) & 0xFF,
			(address.sin_addr.S_un.S_addr >> 24) & 0xFF
		);

		client->data          = NULL;
		client->port          = ntohs (address.sin_port);
		client->size_sent     = 0;
		client->size_received = 0;
	}

	/* initialize closed status */
	if((client->reserved.closed = (void *) CreateEventA (NULL, TRUE, FALSE, NULL)) == NULL)
		goto end_1;

	/* initialize buffer */
	{
		buffer_frame       = (char *) malloc (context->max_size_of_frame    + 4);
		buffer_data_source = (char *) malloc (context->max_size_of_data     + 4);
		buffer_data_target = (char *) malloc (context->max_size_of_data * 2 + 4);
		buffer_data_middle = (char *) malloc (context->max_size_of_data * 2 + 4);

		if(buffer_frame       == NULL)  goto end_2;
		if(buffer_data_source == NULL)  goto end_2;
		if(buffer_data_target == NULL)  goto end_2;
		if(buffer_data_middle == NULL)  goto end_2;
	}

	/* handshake for opening */
	if(ws_util_handshake_open (client->reserved.socket, buffer_frame, context->max_size_of_frame) == 0)
		goto end_2;

	context->on_connect (client);

	/* loop */
	{
		size_t  buffer_frame_size  = 0;

		ws_message_t message;

		message.data    = buffer_data_source;
		message.size    =  0;
		message.binary  = -1;
		message.ansi    = context->ansi;

		while(1)
		{
			size_t  buffer_frame_used  = 0;
			size_t  buffer_frame_want  = 0;

			switch(ws_util_frame (client, & message, & close_code, buffer_frame, buffer_frame_size, & buffer_frame_want, & buffer_frame_used))
			{
				case WS_UTIL_FRAME_STATUS_OK:
					if(ws_util_closed_get (client->reserved.closed) == 0)
					{
						*((wchar_t *) (buffer_data_source + message.size)) = '\0';

						if(message.binary == 0)
						{
							if(utf8_valid (message.text, message.size) == 0)
							{
								close_code = WS_UTIL_CLOSE_INVALID;

								goto end_3;
							}

							if(message.size > 0)
							{
								unsigned int  buffer_data_middle_size = context->max_size_of_data * 2;
								unsigned int  buffer_data_target_size = buffer_data_middle_size;

								if(conv_utf8_to_unic (buffer_data_middle, & buffer_data_middle_size, buffer_data_source, (unsigned int) message.size) == 0)
									goto end_3;

								if(message.ansi)
								{
									if(conv_unic_to_ansi (buffer_data_target, & buffer_data_target_size, buffer_data_middle, buffer_data_middle_size) == 0)
										goto end_3;

									message.data = buffer_data_target;
									message.size = (size_t) buffer_data_target_size;
								}
								else
								{
									message.data = buffer_data_middle;
									message.size = (size_t) buffer_data_middle_size;
								}
							}
						}

						if(context->on_message (client, & message) == 0)
							ws_disconnect (client);
					}

					message.data    = buffer_data_source;
					message.size    =  0;
					message.binary  = -1;

					break;

				case WS_UTIL_FRAME_STATUS_MORE:
					{
						size_t  buffer_frame_read = 0;

						if(buffer_frame_want)
							buffer_frame_read = ws_util_recv ((void *) socket, buffer_frame + buffer_frame_size, buffer_frame_want - buffer_frame_size, 1);
						else
							buffer_frame_read = ws_util_recv ((void *) socket, buffer_frame + buffer_frame_size, context->max_size_of_frame - buffer_frame_size, 0);

						if(buffer_frame_read == 0)
							goto end_3;

						buffer_frame_size     += buffer_frame_read;

						client->size_received += buffer_frame_read;
					}

					break;

				case WS_UTIL_FRAME_STATUS_CONTINUE:
					break;

				case WS_UTIL_FRAME_STATUS_BREAK:
					goto end_3;
			}

			/* shift buffer data to throw the old frame */
			if(buffer_frame_used > 0)
			{
				buffer_frame_size = buffer_frame_size - buffer_frame_used;

				if(buffer_frame_size > 0)
					memcpy (buffer_frame, buffer_frame + buffer_frame_used, buffer_frame_size);
			}
		}
	}

end_3:
	if(ws_util_closed_get (client->reserved.closed) == 0)
		ws_util_handshake_close (client->reserved.socket, close_code);

	context->on_disconnect (client);

end_2:
	if(buffer_frame      )  free (buffer_frame      );
	if(buffer_data_source)  free (buffer_data_source);
	if(buffer_data_target)  free (buffer_data_target);
	if(buffer_data_middle)  free (buffer_data_middle);

	CloseHandle ((HANDLE) client->reserved.closed);
end_1:
	closesocket (socket);
end_0:
	InterlockedDecrement (client->reserved.counter);

	free (client);
}




ws_status_t ws_run (ws_context_t *context)
{
	WSADATA ws_data;
	struct sockaddr_in ws_address;

	SOCKET  socket_server;
	SOCKET  socket_client;

	long *  counter  = (long *) context->reserved.counter;

	if(context == NULL)
		return WS_STATUS_ERROR_ARGUMENT;
	if(context->max_client_number == 0)
		return WS_STATUS_ERROR_ARGUMENT;
	if(context->max_size_of_frame == 0)
		return WS_STATUS_ERROR_ARGUMENT;
	if(context->max_size_of_data  == 0)
		return WS_STATUS_ERROR_ARGUMENT;
	if(context->on_connect    == NULL)
		return WS_STATUS_ERROR_ARGUMENT;
	if(context->on_disconnect == NULL)
		return WS_STATUS_ERROR_ARGUMENT;
	if(context->on_message    == NULL)
		return WS_STATUS_ERROR_ARGUMENT;

	/* initialize client counter (align by sizeof(long)) */
	{
		if(((uintptr_t) counter) % sizeof(long))
			counter = (long *) (((uintptr_t) counter) + (sizeof(long) - (((uintptr_t) counter) % sizeof(long))));

		* counter = 0;
	}

	memset (& ws_data, 0, sizeof ws_data);

	if(WSAStartup (MAKEWORD(2, 2), & ws_data) != 0)
		goto end_1;
	if((socket_server = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		goto end_1;

	if(context->address == NULL)
		ws_address.sin_addr.s_addr = INADDR_ANY;
	else
		ws_address.sin_addr.s_addr = inet_addr (context->address);

	ws_address.sin_family = AF_INET;
	ws_address.sin_port   = htons (context->port);

	if(bind (socket_server, (struct sockaddr *) & ws_address, sizeof ws_address) != 0)
		goto end_2;
	if(listen (socket_server, SOMAXCONN) != 0)
		goto end_2;

	/* store socket into context for ws_terminate() */
	context->reserved.socket = (void *) socket_server;

	while(1)
	{
		ws_client_t * client;

		socket_client = accept (socket_server, NULL, NULL);

		if(socket_client == INVALID_SOCKET)
			break;

		if((size_t) InterlockedIncrement (counter) > context->max_client_number)
			goto reject_this_client;

		if(client = (ws_client_t *) malloc (sizeof(ws_client_t)))
		{
			client->reserved.counter = counter;
			client->reserved.context = context;
			client->reserved.socket  = (void *) socket_client;

			if(_beginthread ((void (__cdecl *)(void *)) ws_run_client_thread, 0, client))
				continue;

			free (client);
		}

reject_this_client:
		InterlockedDecrement (counter);

		closesocket (socket_client);
	}

	closesocket (socket_server);

	return WS_STATUS_OK;

end_2:
	closesocket (socket_server);
end_1:
	return WS_STATUS_ERROR_LISTEN;
}


void ws_terminate (ws_context_t *context)
{
	if(context)
		closesocket ((SOCKET) context->reserved.socket);
}


