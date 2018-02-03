/**
 * \file httpd.h
 * \brief Web Server
 */
#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <stdio.h>
#include <string.h>

/* Logging management */
#define HTTPD_DEBUG

#ifdef HTTPD_DEBUG
  #define httpd_d(fmt, ...)     printf("[httpd] " fmt, ##__VA_ARGS__);
#else
  #define httpd_d(fmt, ...)
#endif

/* ************** Group: Initialization ************** */
/** @name Initialization
 * APIs related to the Initialization of the web server
 * @{
 */


/** Start the Web Server
 *
 * This function starts the web server
 *
 * \return 0 on success, error otherwise
 */
int httpd_start();


/** Stop the Web Server
 *
 * This function stops the web server
 *
 * \return 0 on success, error otherwise
 */
void httpd_stop();
/** End of Group Initialization
 * @}
 */

/* ************** Group: URI Handlers ************** */
/** @name URI Handlers
 * APIs related to the URI handlers
 * @{
 */

/** HTTP Request Type */
typedef enum {
	/** GET request */
	HTTPD_RQTYPE_GET,
	/** POST request */
	HTTPD_RQTYPE_POST,
	/** PUT request */
	HTTPD_RQTYPE_PUT,
} httpd_req_type_t;


typedef void (*httpd_free_sess_ctx_fn_t)(void *sess_ctx);

/** Maximum supported URI length by our HTTPD server */
#define HTTPD_MAX_URI_LEN 256
/** A single HTTPD request */
typedef struct httpd_req {
	/** The type of HTTP request */
	httpd_req_type_t type;
	/** The URI of this request */
	const char       uri[HTTPD_MAX_URI_LEN];
	/** Length of the request body */
	size_t           content_len;
	/** Internally used members */
	void            *aux;
	/** A session context. Contexts are maintained across 'sessions' for a
	 * given open TCP connection. One session could have multiple request
	 * responses. The web server will ensure that the context persists
	 * across all these request and responses.
	 *
	 * By default, this is NULL. URI Handlers can set this to any meaningful
	 * value.
	 *
	 * If the underlying socket gets closed, and this pointer is non-NULL,
	 * the web server will free up the context using the free() call.
	 */
	void            *sess_ctx;
	/** Function to free session context
	 *
	 * If the web server's socket closes, it frees up the session context by
	 * calling free() on the sess_ctx member. If you wish to use a custom
	 * function for freeing the session context, please specify that here.
	 */
	httpd_free_sess_ctx_fn_t free_ctx;
} httpd_req_t;


/** Structure for a URI handler */
struct httpd_uri {
	/** The URI to handle */
	const char *uri;
	/** Handler to call for a GET request. This must return
	 * OS_SUCCESS, or else the underlying socket will be closed. */
	int (*get)(httpd_req_t *req);
	/** Handler to call for a POST request. This must return
	 * OS_SUCCESS, or else the underlying socket will be closed. */
	int (*post)(httpd_req_t *req);
	/** Handler to call for a PUT request. This must return OS_SUCCESS, or
	 * else the underlying socket will be closed. */
	int (*put)(httpd_req_t *req);
};


#define HTTPD_MAX_URI_HANDLERS   8
/** Register a URI handler */
int httpd_register_uri_handler(struct httpd_uri *handler);

/** Unregister a URI handler */
int httpd_unregister_uri_handler(struct httpd_uri *handler);

/** End of URI Handlers
 * @}
 */


/* ************** Group: Request/Response ************** */
/** @name Request / Response
 * APIs related to the data send/receive by URI handlers
 * @{
 */

/** API to send a complete HTTP response
 *
 * This API will send the data as an HTTP response to the
 * request. This assumes that you have the entire response ready in a
 * single buffer. If you wish to send response in incremental segments
 * use httpd_resp_send_chunk() instead.
 *
 * If no status code and content-type were already sent, this will
 * send a 200 OK status code and a content type as text/html. You may
 * call the following functions before this API to configure the
 * response headers
 * - httpd_resp_set_status() - for configuring the HTTP status
 * - httpd_resp_set_type()   - for configuring the Content Type of the
 *                         response
 * - httpd_resp_set_hdr() - for appending any additional field-value
 *                         entries in the response header
 *
 * \note Once this API is called, the request has been responded to. No
 * additional data can then be sent for this request. 
 *
 * \param[in] r The request being responded to
 * \param[in] buf Pointer to a buffer that stores the data
 * \param[in] buf_len Length of the data from the buffer that should
 * be sent out
 *
 * \returns OS_SUCCESS on success. Negative error otherwise.
 */
int httpd_resp_send(httpd_req_t *r, const char *buf, unsigned buf_len);

/** API to send one HTTP chunk
 *
 * This API will send the data as an HTTP response to the
 * request. This API will use chunked-encoding and send the response
 * in the form of chunks. If you have the entire response contained in
 * a single buffer, please use httpd_resp_send() instead.
 *
 * If no status code and content-type were already sent, this will
 * send a 200 OK status code and a content type as text/html. You may
 * call the following functions before this API to configure the
 * response headers
 * - httpd_resp_set_status() - for configuring the HTTP status
 * - httpd_resp_set_type()   - for configuring the Content Type of the
 *                         response
 * - httpd_resp_set_hdr() - for appending any additional field-value
 *                         entries in the response header
 *
 * \note When you are finished sending all your chunks, you must call
 * this function with buf_len as 0.
 *
 * \param[in] r The request being responded to
 * \param[in] buf Pointer to a buffer that stores the data
 * \param[in] buf_len Length of the data from the buffer that should
 * be sent out
 *
 * \returns OS_SUCCESS on success. Negative error otherwise.
 */
int httpd_resp_send_chunk(httpd_req_t *r, const char *buf, unsigned buf_len);

/** HTTP Response 200 */
#define HTTPD_200      "200 OK"
#define HTTPD_204      "204 No Content"
#define HTTPD_207      "207 Multi-Status"
/** HTTP Response 404 */
#define HTTPD_400      "400 Bad Request"
#define HTTPD_404      "404 Not Found"
/** HTTP Response 500 */
#define HTTPD_500      "500 Internal Server Error"

/** API to set the HTTP status code
 *
 * This API sets the status of the HTTP response to the value specified. By
 * default, the '200 OK' response is sent as the response.
 *
 * \note this API only sets the status to this value. The status isn't sent out
 * until any of the send APIs is executed.
 *
 * \param[in] r The request being responded to
 * \param[in] status The HTTP status code of this response
 *
 */
void httpd_resp_set_status(httpd_req_t *r, const char *status);

/** HTTP Content type JSON */
#define HTTPD_TYPE_JSON   "application/json"

/** HTTP Content type text/HTML */
#define HTTPD_TYPE_TEXT   "text/html"

/** HTTP Content type octext-stream */
#define HTTPD_TYPE_OCTET  "application/octet-stream"

/** API to set the HTTP content type
 *
 * This API sets the 'Content Type' field of the response. The default content
 * type is 'text/html'.
 *
 * \note this API only sets the content type to this value. The type isn't sent
 * out until any of the send APIs is executed.
 *
 * \param[in] r The request being responded to
 * \param[in] type The Content Type of the response
 *
 */
void httpd_resp_set_type(httpd_req_t *r, const char *type);

/** API to set any additional headers
 *
 * This API sets any additional header fields that should be sent in
 * the response.
 *
 * \param[in] r The request being responded to
 * \param[in] field The field name of the HTTP header
 * \param[in] value The value of this HTTP header
 *
 * \returns OS_SUCCESS on success. Negative error otherwise.
 */
int httpd_resp_set_hdr(httpd_req_t *r, const char *field, const char *value);

/** API to read some data from the HTTP request
 *
 * This API will read data from the HTTP request into your buffer.
 *
 * \param[in] r The request being responded to
 * \param[in] buf Pointer to a buffer that the data should be read
 * into
 * \param[in] buf_len Maximum length of the buffer pointed to by buf
 *
 * \return The number of bytes read into the buffer
 * \return Zero when no more data is left in this request
 * \return Less than zero on error. If an error is returned, the URI handler
 * must further return an error. This will ensure that the erroneous socket is
 * closed and cleanup up by the web server.
 */
int httpd_req_recv(httpd_req_t *r, char *buf, unsigned buf_len);

/** Get the Socket Descriptor from the HTTP request
 *
 * This API will return the socket descriptor from the HTTP request. You should
 * seldom have to use this API.
 *
 * \param[in] r The request whose socket descriptor should be found
 *
 * \return The socket descriptor for this request
 * \return error otherwise
 */
int httpd_req_to_sockfd(httpd_req_t *r);

/** Helper function for HTTP 404
 *
 * Send HTTP 404 message. If you wish to send additional data in the body of the
 * response, please use the lower-level functions directly.
 *
 * \param[in] r The request being responded to
 *
 * \returns OS_SUCCESS on success. Negative error otherwise.
 */
static inline int httpd_resp_send_404(httpd_req_t *r)
{
#define HTTPD_404_MESSAGE "This URI doesn't exist"
	httpd_resp_set_status(r, HTTPD_404);
	httpd_resp_set_type(r, HTTPD_TYPE_TEXT);
	return httpd_resp_send(r, HTTPD_404_MESSAGE, strlen(HTTPD_404_MESSAGE));
#undef HTTPD_404_MESSAGE
}

/** Raw HTTP send
 *
 * Call this API if you wish to construct your own response headers and
 * everything. For most cases, you wouldn't have to call this API, but you would
 * rather use either of httpd_resp_send() or httpd_resp_send_chunk().
 *
 * If the send override function is set, this API will end up
 * calling that send override function eventually to send data out.
 *
 * \param[in] r The request being responded to
 * \param[in] buf Pointer to a buffer that stores the data
 * \param[in] buf_len Length of the data from the buffer that should
 * be sent out
 *
 * \return the number of bytes that were sent or a negative error
 */
int httpd_send(httpd_req_t *r, const char *buf, unsigned buf_len);

/** Prototype for HTTPDs low-level send function
 *
 * \return the number of bytes sent
 */
typedef int (*httpd_send_func_t)(int sockfd, const char *buf, unsigned buf_len, int flags);

/** Prototype for HTTPDs low-level recv function
 *
 * \return the number of bytes received
 */
typedef int (*httpd_recv_func_t)(int sockfd, char *buf, unsigned buf_len, int flags);

/** Override web server's send function
 *
 * This function overrides the web server's send function. This same function is
 * used to send out any response to any HTTP request.
 *
 * \param[in] r The request being responded to
 * \param[in] send_func The send function to be set for this request
 *
 * \returns OS_SUCCESS on success. Negative error otherwise.
 */
int httpd_set_send_override(httpd_req_t *r, httpd_send_func_t send_func);

/** Override web server's receive function
 *
 * This function overrides the web server's receive function. This same function is
 * used to read and parse HTTP headers as well as body.
 *
 * \param[in] r The request being responded to
 * \param[in] recv_func The receive function to be set for this request
 *
 * \return OS_SUCCESS on success. Negative error otherwise.
 */
int httpd_set_recv_override(httpd_req_t *r, httpd_recv_func_t recv_func);

/** Get session context from socket descriptor
 *
 * Typically if a session context is created, it is available to URI handlers
 * through the httpd_req_t structure. But, there are cases where the web
 * server's send/receive functions may require the context (for example, for
 * accessing keying information etc). Since the send/receive function only have
 * the socket descriptor at their disposal, this API provides them with a way to
 * retrieve the session context.
 *
 * \note There is a proposal to pass the request structure directly to
 * send/receive function, so such APIs may not be required. It will be revisited
 * on-need basis in the future.
 *
 * \param[in] sockfd The socket descriptor for which the context should be
 * extracted.
 *
 * \return The context associated with this session
 */
void *httpd_sess_get_ctx(int sockfd);

/** Get the query paramaters from URL
 *
 * This API can be used to get the various paramaters (name=value pairs)
 * in the received URL.
 *
 * \param[in] r The httpd query structure
 * \param[in] key The key to be searched in the query string
 * \param[out] val Pointer to the buffer into which the value will be
 * copied if the key is found
 * \param[in] val_size Size of the buffer "val"
 *
 * \return OS_SUCCESS if the key is found in the URL
 * \return -OS_FAIL if the key is not found in the URL
 */
int httpd_req_get_url_param(httpd_req_t *r, char *key, char *val, int val_size);

/** End of Request / Response
 * @}
 */

/* ************** Group: Work Queue ************** */
/** @name Work Queue
 * APIs related to the HTTPD Work Queue
 * @{
 */

/** Prototype of the HTTPD work function
 *
 * Please refer to httpd_queue_work() for more details.
 *
 * \param[in] arg The arguments for this work function
 */
typedef void (*httpd_work_fn_t)(void *arg);

/** Execute function in HTTPD's context
 *
 * \note For the most part you shouldn't have to use this function. Some
 * protocols require that the web server generate some asynchronous data and
 * send it to the persistently opened connection. This facility is for use by
 * these protocols.
 *
 * \param[in] work Pointer to the function to be executed in the HTTPD's context
 * \param[in] arg Pointer to the arguments that should be passed to this
 * function
 *
 * \return OS_SUCCESS on successfully queueing the work
 * \return error otherwise
 */
int httpd_queue_work(httpd_work_fn_t work, void *arg);

/** End of Group Work Queue
 * @}
 */

#endif /* ! _HTTPD_H_ */
