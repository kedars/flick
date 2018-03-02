#include <stdlib.h>
#include <httpd.h>

#include "httpd_priv.h"

#define HDR_CONTENT_LEN   "Content-Length:"
static int httpd_parse_hdr_field(httpd_req_t *r,
				 struct httpd_req_aux *ra,
				 char *buf, int buf_len)
{
	/* buf[buf_len]     is \0
	 * buf[buf_len - 1] is \n
	 * buf[buf_len - 2] is \r
	 * -and before this is the value.
	 *
	 * For convenience, let's replace \r with \0
	 */
	buf[buf_len - 2] = '\0';
	if (strncmp(HDR_CONTENT_LEN, buf, strlen(HDR_CONTENT_LEN)) == 0) {
		char *p = buf + strlen(HDR_CONTENT_LEN);
		char *endptr;
		r->content_len = strtol(p, &endptr, 10);
		if (*endptr != '\0')
			return -OS_FAIL;
		ra->remaining_len = r->content_len;
	}
	return OS_SUCCESS;
}

static int httpd_parse_first_line(httpd_req_t *r, char *buf, int buf_len)
{
	/* Since this is a line from the HTTP header, the last byte is \n. We
	 * will overwrite that with \0 so that we can use str*() functions
	 * without spilling over.
	 */
	char *current = buf;
	buf[buf_len - 1] = '\0';


	/* Extract method */
	char *token = strsep(&current, " ");
	if (strcmp(token, "GET") == 0) {
		r->type = HTTPD_RQTYPE_GET;
	} else if (strcmp(token, "POST") == 0) {
		r->type = HTTPD_RQTYPE_POST;
	} else if (strcmp(token, "PUT") == 0) {
		r->type = HTTPD_RQTYPE_PUT;
	} else {
		httpd_d("HTTP Operation not supported\n");
		r->type = -1;
		/* Continue instead of returning error. It will return 404 if
		 * URI match doesn't work */
	}


	/* Extract URI */
	if (!current || *current == '\0')
		return -OS_FAIL;
	token = strsep(&current, " ");
	strncpy((char *)r->uri, token, sizeof(r->uri));


	/* Extract version */
	if (*current == '\0')
		return -OS_FAIL;
	token = strsep(&current, "\r");
	if (strcmp(token, "HTTP/1.1") != 0) {
		httpd_d("Unsupported HTTP version\n");
		return -OS_FAIL;
	}
	return OS_SUCCESS;
}

static int httpd_read_one_line(httpd_req_t *r, struct httpd_req_aux *ra)
{
	int ret;
	unsigned i = 0;
	bool cr_received = false;

	while (i < (sizeof(ra->scratch) - 2)) {
		ret = httpd_recv(r, ra->scratch + i, 1);
		if (ret < 0)
			return ret;

		if (ra->scratch[i] == '\n' && cr_received) {
			/* Return the number of bytes read so far */
			return i + 1;
		}

		if (ra->scratch[i] == '\r')
			cr_received = true;
		else
			cr_received = false;

		i++;
	}
	httpd_d("Header too long\n");
	return -OS_FAIL;
}

int httpd_parse_hdrs(httpd_req_t *r, struct httpd_req_aux *ra)
{
	int rd_bytes, ret;
	bool first_line = false;

	while (1) {
		rd_bytes = httpd_read_one_line(r, ra);
		if (rd_bytes < 0)
			return rd_bytes;
//		httpd_d("Line read:%.*s:\n", rd_bytes, ra->scratch);

		if (rd_bytes == 2 &&
		    ra->scratch[0] == '\r' &&
		    ra->scratch[1] == '\n')
			break;

		if (! first_line) {
			ret = httpd_parse_first_line(r, ra->scratch, rd_bytes);
			if (ret < 0)
				return ret;
			first_line = true;
			httpd_d("URI: %s\n", r->uri);
		} else {
			/* Instead of parsing this here, we could perform
			 * a delayed parsing, in case the URI handler is
			 * interested in any of the header files.
			 */
			ret = httpd_parse_hdr_field(r, ra, ra->scratch, rd_bytes);
			if (ret < 0)
				return ret;
		}
	}
	return OS_SUCCESS;
}

/* Get a URL query tag from a URL of the type /resource?param1=val1&param2=val2
 */
int httpd_req_get_url_param(httpd_req_t *r, char *key, char *val, int val_size)
{
	/* Copy the URI into another buffer because strsep() modifies the
	 * string passed to it
	 */
	char url[HTTPD_MAX_URI_LEN];
	strncpy(url, r->uri, HTTPD_MAX_URI_LEN);
	char *url_ptr = url;
	/* Check for the presence of '?' in the URI which indicates that
	 * there is a query string. url_ptr will point to the first character
	 * after '?' or to the terminating '\0' depending on the presence/absence
	 * of a query string.
	 */
	char *p = strsep(&url_ptr, "?");

	while (url_ptr && strlen(url_ptr)) {
		/* Get the name=val string. Multiple name=value pairs
		 * are separated by '&'
		 */
		p = strsep(&url_ptr, "&");

		/* Search for the '=' character. Else, it would mean
		 * that the parameter is invalid
		 */
		char *val_ptr = strchr(p, '=');
		if (!val_ptr)
			break;

		/* If the key, does not match, continue searching */
		if (strncmp(p, key, strlen(key)))
			continue;

		/* Else, get the NULL terminated value and copy it to the
		 * caller's buffer.
		 * However, if the length is 0, return failure.
		 */
		val_ptr++;
		if (!strlen(val_ptr))
			return -OS_FAIL;

		strncpy(val, val_ptr, val_size);
		return OS_SUCCESS;
	}
	httpd_d("Key %s not found\n", key);
	return -OS_FAIL;
}

/* This (request management) could probably be a file in itself, let's see */
int httpd_req_new(httpd_req_t *r, struct sock_db *sd)
{
	memset(r, 0, sizeof(hd.hd_req));
	memset(&hd.hd_req_aux, 0, sizeof(hd.hd_req_aux));
	r->aux = &hd.hd_req_aux;
	/* Associate the request to the socket */
	struct httpd_req_aux *ra  = r->aux;
	ra->sd = sd;
	/* Set defaults */
	ra->status = HTTPD_200;
	ra->content_type = HTTPD_TYPE_JSON;
	/* Copy session info to the request */
	r->sess_ctx = sd->ctx;
	r->free_ctx = sd->free_ctx;
	return httpd_parse_hdrs(r, r->aux);
}

int httpd_req_delete(httpd_req_t *r)
{
	struct httpd_req_aux *ra = r->aux;
	/* Finish off reading any pending/leftover data */
	while (ra->remaining_len) {
		/* Any length small enough not to overload the stack, but large
		 * enough to finish off the buffers fast
		 */
		char dummy[25];
		int recv_len = (sizeof(dummy) > ra->remaining_len) ?
			ra->remaining_len : sizeof(dummy);
		int ret = httpd_req_recv(r, dummy, recv_len);
		if (ret <  0)
			return ret;
	}
	/* Retrieve session info from the request into the socket database */
	ra->sd->ctx = r->sess_ctx;
	ra->sd->free_ctx = r->free_ctx;
	/* Clear out the request and request_aux structures */
	ra->sd = NULL;
	r->aux = NULL;
	return OS_SUCCESS;
}
