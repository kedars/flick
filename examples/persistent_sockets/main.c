#include <stdlib.h>

#include <osal.h>
#include <httpd.h>

/* An example to demonstrate persistent sockets, with context maintained across
 * multiple requests on that socket.
 */

/* This handler keeps accumulating data that is posted to it into a per
 * socket/session context. And returns the result.
 */
int adder_post_handler(httpd_req_t *req)
{
	char buf[10];
	char outbuf[50];
	int ret;

	/* Read data received in the request */
	ret = httpd_req_recv(req, buf, sizeof(buf));
	if (ret < 0)
		return -OS_FAIL;
	buf[ret] = '\0';
	int val = atoi(buf);
	printf("/adder handler read %d\n", val);

	/* Create session's context if not already available */
	if (! req->sess_ctx) {
		printf("/adder allocating new session\n");
		req->sess_ctx = malloc(sizeof(int));
		*(int *)req->sess_ctx = 0;
	}

	/* Add the received data to the context */
	int *adder = (int *)req->sess_ctx;
	*adder += val;

	/* Respond with the accumulated value */
	snprintf(outbuf, sizeof(outbuf),"%d", *adder);
	httpd_resp_send(req, outbuf, strlen(outbuf));
	return OS_SUCCESS;
}

struct httpd_uri adder_handler = {
	.uri = "/adder",
	.post = adder_post_handler
};

int main()
{
     /* Start the web server */
     httpd_start();
     httpd_register_uri_handler(&adder_handler);

     /* The httpd_start(), starts a new thread. Make sure the process doesn't
      * terminate killing that thread
      */
     pthread_exit(NULL);
     return 0;
}
