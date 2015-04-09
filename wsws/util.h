/*
	[WSWS]  /util.h , 150406   (C) 2005-2015 MF
*/


#ifndef _WSWS_UTIL_H_
#define _WSWS_UTIL_H_



#ifdef __cplusplus
	extern "C" {
#endif



typedef enum
{
	WS_UTIL_CLOSE_NULL      =    0,
	WS_UTIL_CLOSE_OK        = 1000,
	WS_UTIL_CLOSE_ERROR     = 1002,
	WS_UTIL_CLOSE_INVALID   = 1007,
	WS_UTIL_CLOSE_TOOLARGE  = 1009,
}
ws_util_close_e;


typedef enum
{
	WS_UTIL_FRAME_STATUS_OK,
	WS_UTIL_FRAME_STATUS_MORE,
	WS_UTIL_FRAME_STATUS_CONTINUE,
	WS_UTIL_FRAME_STATUS_BREAK,
}
ws_util_frame_status_e;



ws_util_frame_status_e  ws_util_frame (ws_client_t *client, ws_message_t *message, ws_util_close_e *close, char *buffer, size_t buffer_size, size_t *buffer_want, size_t *buffer_used);


void   ws_util_unmask  (char *target, char *source, size_t size, uint32_t mask);

int    ws_util_closed_get  (void *closed);
void   ws_util_closed_set  (void *closed);

int    ws_util_send  (void *socket, const char *buffer, size_t buffer_size);
size_t ws_util_recv  (void *socket, char *buffer, size_t buffer_size, int all);

void   ws_util_pong  (void *socket, uint8_t *ping, size_t payload_size);


int  ws_util_handshake_open   (void *socket, char *buffer, size_t buffer_size);
void ws_util_handshake_close  (void *socket, ws_util_close_e code);



#ifdef __cplusplus
	}
#endif


#endif   /* _WSWS_UTIL_H_ */