#include <osal.h>
#include <httpd.h>

/* A simple example that demonstrates how to create GET and POST handlers for
 * the web server.
 */
#define HELLO_WORLD "Hello World!"

/* An HTTP GET handler */
int hello_get_handler(httpd_req_t *req)
{
	/* Send a simple response */
	httpd_resp_send(req, HELLO_WORLD, strlen(HELLO_WORLD));
	return OS_SUCCESS;
}

struct httpd_uri get_handler = {
	.uri = "/hello",
	.get = hello_get_handler
};

/* An HTTP POST handler */
int echo_post_handler(httpd_req_t *req)
{
	char buf[100];
	int ret;

	/* Read the data for the request */
	ret = httpd_req_recv(req, buf, sizeof(buf));
	if (ret < 0)
		return -OS_FAIL;

	buf[ret] = '\0';
	/* Send back the same data */
	httpd_resp_send(req, buf, strlen(buf));
	return OS_SUCCESS;
}

struct httpd_uri post_handler = {
	.uri = "/echo",
	.post = echo_post_handler
};

int main()
{
     /* Start the web server */
     httpd_start();
     httpd_register_uri_handler(&get_handler);
     httpd_register_uri_handler(&post_handler);

     /* The httpd_start(), starts a new thread. Make sure the process doesn't
      * terminate killing that thread
      */
     othread_delete();
     return 0;
}
