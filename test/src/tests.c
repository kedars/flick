#include <osal.h>
#include <httpd.h>

#include <stdlib.h>
#include <stdbool.h>

int pre_start_mem, post_stop_mem, post_stop_min_mem;
bool basic_sanity = true;

/********************* Basic Handlers Start *******************/
int hello_get_handler(httpd_req_t *req)
{
#define STR "Hello World!"
	httpd_resp_send(req, STR, strlen(STR));
	return OS_SUCCESS;
#undef STR
}

int hello_type_get_handler(httpd_req_t *req)
{
#define STR "Hello World!"
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, STR, strlen(STR));
	return OS_SUCCESS;
#undef STR
}

int hello_status_get_handler(httpd_req_t *req)
{
#define STR "Hello World!"
	httpd_resp_set_status(req, HTTPD_500);
	httpd_resp_send(req, STR, strlen(STR));
	return OS_SUCCESS;
#undef STR
}

int echo_post_handler(httpd_req_t *req)
{
	char buf[100];
	int ret;

	ret = httpd_req_recv(req, buf, sizeof(buf));
	if (ret < 0)
		return -OS_FAIL;

	buf[ret] = '\0';
	httpd_resp_send(req, buf, strlen(buf));
	return OS_SUCCESS;
}

void adder_free_func(void *ctx)
{
	printf("Custom Free Context function called\n");
	free(ctx);
}
/* Create a context, keep incrementing value in the context, by whatever was
 * received. Return the result
 */
int adder_post_handler(httpd_req_t *req)
{
	char buf[10];
	char outbuf[50];
	int ret;

	ret = httpd_req_recv(req, buf, sizeof(buf));
	if (ret < 0)
		return -OS_FAIL;
	buf[ret] = '\0';
	int val = atoi(buf);

	printf("/adder handler read %d\n", val);
	if (! req->sess_ctx) {
		printf("/adder allocating new session\n");
		req->sess_ctx = malloc(sizeof(int));
		req->free_ctx = adder_free_func;
		*(int *)req->sess_ctx = 0;
	}
	int *adder = (int *)req->sess_ctx;
	*adder += val;

	snprintf(outbuf, sizeof(outbuf),"%d", *adder);
	httpd_resp_send(req, outbuf, strlen(outbuf));
	return OS_SUCCESS;
}

int leftover_data_post_handler(httpd_req_t *req)
{
	/* Only echo the first 10 bytes of the request, leaving the rest of the
	 * request data as is.
	 */
	char buf[11];
	int ret;

	ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
	if (ret < 0)
		return -OS_FAIL;

	buf[ret] = '\0';
	httpd_resp_send(req, buf, strlen(buf));
	return OS_SUCCESS;
}


struct httpd_uri basic_handlers[] = {
	{ .uri = "/hello/type_html",
	  .get = hello_type_get_handler,
	},
	{ .uri = "/hello",
	  .get = hello_get_handler,
	},
	{ .uri = "/hello/status_500",
	  .get = hello_status_get_handler,
	},
	{ .uri = "/echo",
	  .post = echo_post_handler,
	  .put  = echo_post_handler,
	},
	{ .uri = "/leftover_data",
	  .post = leftover_data_post_handler,
	},
	{ .uri = "/adder",
	  .post = adder_post_handler,
	},
};

int basic_handlers_no = sizeof(basic_handlers)/sizeof(struct httpd_uri);
void register_basic_handlers(void)
{
	int i, ret;
	printf("No of handlers = %d\n", basic_handlers_no);
	for (i = 0; i < basic_handlers_no; i++) {
		ret = httpd_register_uri_handler(&basic_handlers[i]);
		printf("register uri returned %d\n", ret);
	}
}
/********************* Basic Handlers End *******************/


int my_hello_post_handler(httpd_req_t *req)
{
	char buf[10];
	char outbuf[50];
	int ret;

	ret = httpd_req_recv(req, buf, sizeof(buf));
	if (ret < 0)
		return -OS_FAIL;

	httpd_resp_set_status(req, HTTPD_404);
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	printf("Read %d bytes as:%s:\n", ret, buf);
	buf[ret] = '\0';
#define STR "my_hello_handler"
	snprintf(outbuf, sizeof(outbuf), STR" %s", buf);
	httpd_resp_send(req, outbuf, strlen(outbuf));
	return OS_SUCCESS;
#undef STR
}


/********************* Test Handler Limit Start *******************/
int null_func(httpd_req_t *req)
{
	return OS_SUCCESS;
}

struct httpd_uri handler_limit_uri = {
	.uri = "/limit",
	.get = null_func,
	.post = null_func
};

void test_handler_limit(void)
{
	int i, ret;

	/* Register the same handler for MAX URI Handlers */
	printf("Test: Register Max URI handlers: %d...", HTTPD_MAX_URI_HANDLERS);
	for (i = 0; i < HTTPD_MAX_URI_HANDLERS; i++) {
		ret = httpd_register_uri_handler(&handler_limit_uri);
		if (ret != OS_SUCCESS) {
			printf("Fail\n");
			goto error_ret;
		}
	}
	printf("Success\n");

	/* Register the MAX URI + 1 Handlers should fail */
	printf("Test: Register Max URI + 1 handlers: %d th...", HTTPD_MAX_URI_HANDLERS +1 );
	ret = httpd_register_uri_handler(&handler_limit_uri);
	if (ret == OS_SUCCESS) {
		printf("Fail\n");
		goto error_ret;
	}
	printf("Success\n");

	/* Register the MAX URI + 1 Handlers should fail */
	printf("Test: Unregister  1 handlers...");
	ret = httpd_unregister_uri_handler(&handler_limit_uri);
	if (ret != OS_SUCCESS) {
		printf("Fail\n");
		goto error_ret;
	}
	printf("Success\n");

	/* Register the MAX URI + 1 Handlers should fail */
	printf("Test: Register 1 more handler");
	ret = httpd_register_uri_handler(&handler_limit_uri);
	if (ret != OS_SUCCESS) {
		printf("Fail\n");
		goto error_ret;
	}
	printf("Success\n");

	/* Unregister the same handler for MAX URI Handlers */
	printf("Test: Unregister all handlers:");
	for (i = 0; i < HTTPD_MAX_URI_HANDLERS; i++) {
		ret = httpd_unregister_uri_handler(&handler_limit_uri);
		if (ret != OS_SUCCESS) {
			printf("Fail\n");
			goto error_ret;
		}
	}
	printf("Success\n");
error_ret:
	basic_sanity = false;
}

/********************* Test Handler Limit End *******************/

int test_httpd_start()
{
	pre_start_mem = os_get_current_free_mem();
	printf("HTTPD Start: Current free memory: %d\n", pre_start_mem);
	return httpd_start();
}

void test_httpd_stop()
{
	httpd_stop();
	post_stop_mem = os_get_current_free_mem();
	post_stop_min_mem = os_get_minimum_free_mem();
	httpd_d("HTTPD Stop: Current free memory: %d\n", post_stop_mem);
	httpd_d("HTTPD Stop: Minimum free memory: %d\n", post_stop_min_mem);
}

void start_tests(void)
{
	test_handler_limit();

	printf("Starting httpd\n");
	test_httpd_start();

	register_basic_handlers();
}
