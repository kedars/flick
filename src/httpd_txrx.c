#include <errno.h>
#include <httpd.h>

#include "httpd_priv.h"

int httpd_set_send_override(httpd_req_t *r, httpd_send_func_t send_func)
{
	struct httpd_req_aux *ra = r->aux;
	ra->sd->send_fn = send_func;
	return OS_SUCCESS;
}

int httpd_set_recv_override(httpd_req_t *r, httpd_recv_func_t recv_func)
{
	struct httpd_req_aux *ra = r->aux;
	ra->sd->recv_fn = recv_func;
	return OS_SUCCESS;
}

int httpd_send(httpd_req_t *r, const char *buf, unsigned buf_len)
{
	struct httpd_req_aux *ra = r->aux;
	return ra->sd->send_fn(ra->sd->fd, buf, buf_len, 0);
}

int httpd_recv(httpd_req_t *r, char *buf, unsigned buf_len)
{
	struct httpd_req_aux *ra = r->aux;
	int ret = ra->sd->recv_fn(ra->sd->fd, buf, buf_len, 0);
	if (ret == 0)
		ret = -ECONNRESET;
	return ret;
}


#define HTTPD_HDR_STR      "HTTP/1.1 %s\r\n"                   \
                           "Content-Type: %s\r\n"              \
                           "Content-Length: %d\r\n"
int httpd_resp_send(httpd_req_t *r, const char *buf, unsigned buf_len)
{
	struct httpd_req_aux *ra = r->aux;
	snprintf(ra->scratch, sizeof(ra->scratch), HTTPD_HDR_STR,
		 ra->status, ra->content_type, buf_len);
	httpd_send(r, ra->scratch, strlen(ra->scratch));
	/* Space for sending additional headers based on set_header */
	httpd_send(r, "\r\n", strlen("\r\n"));
	if (buf && buf_len)
	     httpd_send(r, buf, buf_len);
	return OS_SUCCESS;
}

void httpd_resp_set_status(httpd_req_t *r, const char *status)
{
	struct httpd_req_aux *ra = r->aux;
	ra->status = (char *)status;
}

void httpd_resp_set_type(httpd_req_t *r, const char *type)
{
	struct httpd_req_aux *ra =r->aux;
	ra->content_type = (char *)type;
}

int httpd_req_recv(httpd_req_t *r, char *buf, unsigned buf_len)
{
	struct httpd_req_aux *ra = r->aux;
	if (buf_len > ra->remaining_len)
		buf_len = ra->remaining_len;
	if (buf_len == 0)
		return buf_len;

	int ret = httpd_recv(r, buf, buf_len);
	if (ret < 0) {
		ra->remaining_len = 0;
		return -OS_FAIL;
	}
	ra->remaining_len -= ret;
	return ret;
}

int httpd_req_to_sockfd(httpd_req_t *r)
{
	struct httpd_req_aux *ra = r->aux;
	return ra->sd->fd;
}

int __httpd_send(int sockfd, const char *buf, unsigned buf_len, int flags)
{
	return send(sockfd, buf, buf_len, flags);
}

int __httpd_recv(int sockfd, char *buf, unsigned buf_len, int flags)
{
	return recv(sockfd, buf, buf_len, flags);
}
