# Flick: A tiny web server for embedded systems

## Features
* Low-footprint for microcontroller usage scenario
* No dynamic allocations
* Portable across Linux and RTOS platforms
  * Unix (Mac / Linux)
  * [ESP-32](examples/esp32)
* Supports HTTP/1.1
* Registration of URI handlers for GET, PUT and POST requests
* Supports HTTP pipelining (multiple requests on the same socket)
* Supports persistent sockets with context preserved across multiple requests
* Supports multiple open connections at the same time
* Is single-threaded, so a single connection is served at a given time
* Allows per-socket overriding of the Web Server's send/receive functions

## Notes
* The webserver is just out of the oven, do let me know in case of any issues/code-review reports
* Would love to hear any other features requests
* Hasn't been reviewed by any other person, any comments would be helpful

## Usage

* Write a function handler for a URI GET or POST
* Start the web server
* Register the URI handler, as below

```c
/* Write a GET handler */
int hello_get_handler(httpd_req_t *req)
{
	char a[] = "Hello World!";
	/* Send a simple response */
	httpd_resp_send(req, a, strlen(a));
	return OS_SUCCESS;
}

struct httpd_uri get_handler = {
	.uri = "/hello",
	.get = hello_get_handler
};

int main()
{
     /* Start the web server */
     httpd_start();
     /* Register the handler */
     httpd_register_uri_handler(&get_handler);
}

```

## Give it a spin!

* The API is available at: [include/httpd.h](include/httpd.h)
* Look at an example: [examples/simple/](examples/simple)
* Run the tests: [test/](test/)

