/*
	[WSWS]  /wsws.h , 150406   (C) 2005-2015 MF
*/


#ifndef _WSWS_H_
#define _WSWS_H_


#include <stdint.h>


#ifdef __cplusplus
	extern "C" {
#endif



/* a ws_client_t represents one client connection */
typedef struct
{
	struct
	{
		long * counter;
		void * context;
		void * socket;
		void * closed;
	}
	/* for internal used, DO NOT MODIFY ME ! */
	reserved;

	/* you can use it to store any thing */
	void *   data;

	/* ip address of client connection */
	char     address [24];
	/* port number of client connection */
	uint16_t port;

	/* data size in bytes sent to client */
	size_t   size_sent;
	/* data size in bytes received from client */
	size_t   size_received;
}
ws_client_t;



/* a ws_message_t represents one message received from client */
typedef struct
{
	union
	{
		const void *    data;     /* pointer for binary data */
		const char *    text;     /* pointer for ansi text */
		const wchar_t * textw;    /* pointer for unicode text */
	};

	/* size of binary data, or length of text */
	size_t  size;

	/* 0=text, 1=binary data */
	int  binary;
	/* 0=unicode text, 1=ansi text (available only if ws_message_t::binary = 0) */
	int  ansi;
}
ws_message_t;



typedef void * (__cdecl * ws_connect_cb   ) (const ws_client_t *client);
typedef void   (__cdecl * ws_disconnect_cb) (const ws_client_t *client);
typedef int    (__cdecl * ws_message_cb   ) (const ws_client_t *client, const ws_message_t *message);



/* a ws_context_t represents one server */
typedef struct
{
	struct
	{
		void *  socket;
		uint8_t counter [16];
	}
	/* for internal used, DO NOT MODIFY ME ! */
	reserved;

	/* host address to listen (NULL equals INADDR_ANY) */
	char *   address;
	/* port number to listen */
	uint16_t port;

	/* set ANSI/UNICODE text pass to ws_context_t::on_message() */
	int ansi;

	/* max number of clients  */
	size_t  max_client_number;

	/* max frame size in bytes received from clients */
	size_t  max_size_of_frame;
	/* max data size in bytes received from clients */
	size_t  max_size_of_data;

	/* called when finishing handshake with a client connection
	   this callback function SHOULD BE thread safe
	   return data will be stored into ws_client_t::data */
	ws_connect_cb    on_connect;
	/* called when a client connection is closed
	   this callback function SHOULD BE thread safe
	   you SHOULD NOT send any data to client in this function*/
	ws_disconnect_cb on_disconnect;
	/* called when some data received from client
	   this callback function SHOULD BE thread safe
	   you can return a zero/nonzero value to close/keep the connection with client */
	ws_message_cb    on_message;
}
ws_context_t;



/* server status */
typedef enum
{
	WS_STATUS_OK,
	WS_STATUS_ERROR_ARGUMENT,
	WS_STATUS_ERROR_LISTEN
}
ws_status_t;




/* run a server */
ws_status_t  ws_run  (ws_context_t *context);

/* terminate a server (this function WILL NOT close any client connection) */
void ws_terminate    (ws_context_t *context);

/* send binary data to client */
int  ws_send_binary  (const ws_client_t *client, const void *buffer, size_t buffer_size);
/* send text to client (text_length will be calculated automatically if text_length < 0) */
int  ws_send_text    (const ws_client_t *client, const void *text, int text_length, int ansi);

/* disconnect with a client */
void ws_disconnect   (const ws_client_t *client);



#ifdef __cplusplus
	}
#endif


#endif   /* _WSWS_H_ */