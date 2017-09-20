#include <errno.h>

#include <httpd.h>

#include "httpd_priv.h"

#define httpd_uri_d(...) 
// #define httpd_uri_d httpd_d

int httpd_register_uri_handler(struct httpd_uri *handle)
{
	int i;
	for (i = 0; i < HTTPD_MAX_URI_HANDLERS; i++) {
		httpd_uri_d("[%d]", i);
		if (hd.hd_calls[i] == NULL) {
			httpd_uri_d ("installed\n");
			hd.hd_calls[i] = handle;
			return OS_SUCCESS;
		}
		httpd_uri_d("exists %s\n", hd.hd_calls[i]->uri);
	}
	return -ENOSPC;
}

int httpd_unregister_uri_handler(struct httpd_uri *handle)
{
	int i;
	for (i = 0; i < HTTPD_MAX_URI_HANDLERS; i++) {
		if (hd.hd_calls[i] == handle) {
			hd.hd_calls[i] = NULL;
			return OS_SUCCESS;
		}
	}
	return -EINVAL;
}

static int get_match_len(const char *s1, const char *s2)
{
	int cnt = 0;
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
		cnt++;
	}
	return cnt;
}

typedef int (*httpd_uri_handler_t)(httpd_req_t *r);


/* Assumes that req->uri begins with /, some clients send request
 * starting with 'http://', take care of that right at the time of
 * header parsing
 */
static httpd_uri_handler_t httpd_find_handler(httpd_req_t *req)
{
	int i, cur_match = -1, cur_match_len = 0;
	for (i = 0; i < HTTPD_MAX_URI_HANDLERS; i++) {
		if (hd.hd_calls[i]) {
			httpd_uri_d("hdlr [%d] = %s\n", i, hd.hd_calls[i]->uri);
			int tmp = get_match_len(hd.hd_calls[i]->uri, req->uri);
			if (tmp > cur_match_len &&
			    (unsigned )tmp == strlen(hd.hd_calls[i]->uri)) {
				cur_match_len = tmp;
				cur_match = i;
			}
		}
	}
	if (cur_match == -1)
		return NULL;

	switch (req->type) {
	case HTTPD_RQTYPE_GET:
		return hd.hd_calls[cur_match]->get;
		break;
	case HTTPD_RQTYPE_POST:
		return hd.hd_calls[cur_match]->post;
		break;
	case HTTPD_RQTYPE_PUT:
		return hd.hd_calls[cur_match]->put;
		break;
	}
	return NULL;
}

int httpd_uri(httpd_req_t *req)
{
	httpd_uri_handler_t uri_handler;

	httpd_uri_d("Request %d for %s\n", req->type, req->uri);
	uri_handler = httpd_find_handler(req);
	if (uri_handler == NULL) {
		httpd_uri_d("Response: 404\n");
		httpd_resp_send_404(req);
		goto out;
	}
	if (uri_handler(req) != OS_SUCCESS) {
		/* Something failed, this socket should be closed */
		return -OS_FAIL;
	}

 out:
	/* XXX Cleanup the rest of the body of the request */
	return OS_SUCCESS;
}
