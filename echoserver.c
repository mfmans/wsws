/*
	[WSWS]  /echoserver.c , 150406   (C) 2005-2015 MF
*/


#pragma comment (lib, "WS2_32")


#include <stdio.h>
#include <locale.h>
#include <Windows.h>

#include "wsws/wsws.h"



void * __cdecl echoserver_on_connect (const ws_client_t *client)
{
	printf ("<%s:%u> Connection is established\n", client->address, client->port);

	return NULL;
}


void __cdecl echoserver_on_disconnect (const ws_client_t *client)
{
	printf ("<%s:%u> Connection is closed (sent: %u B, received: %u B)\n", client->address, client->port, client->size_sent, client->size_received);
}


int __cdecl echoserver_on_message (const ws_client_t *client, const ws_message_t *message)
{
	if(message->binary)
	{
		printf ("<%s:%u> Binary data received (%u Bytes)\n", client->address, client->port, message->size);

		ws_send_binary (client, message->data, message->size);
	}
	else
	{
		printf ("<%s:%u> Text received (%u Words)\n", client->address, client->port, message->size);

		ws_send_text (client, message->text, (int) message->size, message->ansi);
	}

	return 1;
}




int main (int argc, char *argv[])
{
	ws_context_t ws;

	memset (& ws, 0, sizeof ws);

	ws.address   = NULL;
	ws.port      = 5555;
	ws.ansi      = 0;
	ws.max_client_number  = 10;
	ws.max_size_of_frame  = 32 * 1024 * 1024;
	ws.max_size_of_data   = 32 * 1024 * 1024;
	ws.on_connect    = echoserver_on_connect;
	ws.on_disconnect = echoserver_on_disconnect;
	ws.on_message    = echoserver_on_message;

	/* ignore CTRL+C input */
	SetConsoleCtrlHandler (NULL, TRUE);

	/* for %ls in printf() */
	setlocale (LC_ALL, "");

	switch(ws_run (& ws))
	{
		case WS_STATUS_OK:
			break;

		case WS_STATUS_ERROR_ARGUMENT:
			printf ("[ERROR] ws_run() : WS_STATUS_ERROR_ARGUMENT\n");
			break;

		case WS_STATUS_ERROR_LISTEN:
			printf ("[ERROR] ws_run() : WS_STATUS_ERROR_LISTEN\n");
			break;
	}

	/* if you call ws_terminate() to stop the server,
	   you should close all client connection by yourself */

	return 0;
}


