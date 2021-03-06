#include "rtsp-server.h"
#include "rtsp-reason.h"
#include "rfc822-datetime.h"
#include "rtsp-server-internal.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct rtsp_server_t* rtsp_server_create(const char ip[65], unsigned short port, struct rtsp_handler_t* handler, void* ptr, void* ptr2)
{
	struct rtsp_server_t* rtsp;

	rtsp = (struct rtsp_server_t *)malloc(sizeof(struct rtsp_server_t));
	if (NULL == rtsp) return NULL;

	snprintf(rtsp->ip, sizeof(rtsp->ip), "%s", ip);
	rtsp->port = port;
	rtsp->param = ptr;
	rtsp->sendparam = ptr2;
	memcpy(&rtsp->handler, handler, sizeof(rtsp->handler));
	rtsp->parser = rtsp_parser_create(RTSP_PARSER_SERVER);
	return rtsp;
}

int rtsp_server_destroy(struct rtsp_server_t* rtsp)
{
	if (rtsp->parser)
	{
		rtsp_parser_destroy(rtsp->parser);
		rtsp->parser = NULL;
	}

	free(rtsp);
	return 0;
}

int rtsp_server_input(struct rtsp_server_t* rtsp, const void* data, size_t bytes)
{
	int r, remain;

	remain = (int)bytes;
	do
	{
		if (*(char*)data == '$')
		{
			// TODO: rtsp over tcp
			assert(0);
		}

		r = rtsp_parser_input(rtsp->parser, (const char*)data + (bytes - remain), &remain);
		assert(r <= 1); // 1-need more data
		if (0 == r)
		{
			r = rtsp_server_handle(rtsp);
			rtsp_parser_clear(rtsp->parser); // reset parser
		}
	} while (remain > 0 && r >= 0);

	assert(r <= 1);
	return r >= 0 ? 0 : r;
}

const char* rtsp_server_get_header(struct rtsp_server_t *rtsp, const char* name)
{
	return rtsp_get_header_by_name(rtsp->parser, name);
}

const char* rtsp_server_get_client(rtsp_server_t* rtsp, unsigned short* port)
{
	if (port) *port = rtsp->port;
	return rtsp->ip;
}

int rtsp_server_reply(struct rtsp_server_t *rtsp, int code)
{
	int len;
	rfc822_datetime_t datetime;
	rfc822_datetime_format(time(NULL), datetime);

	len = snprintf(rtsp->reply, sizeof(rtsp->reply),
		"RTSP/1.0 %d %s\r\n"
		"CSeq: %u\r\n"
		"Date: %s\r\n"
		"\r\n",
		code, rtsp_reason_phrase(code), rtsp->cseq, datetime);

	return rtsp->handler.send(rtsp->sendparam, rtsp->reply, len);
}
