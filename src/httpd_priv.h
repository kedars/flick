#ifndef _HTTPD_PRIV_H_
#define _HTTPD_PRIV_H_

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <osal.h>

/* The maximum number of sockets that will stay in the open state */
#define HTTPD_MAX_OPEN_SOCKETS 8
#define HTTPD_SCRATCH_BUF      512

struct thread_data {
	othread_t      handle;
	enum {
		THREAD_IDLE = 0,
		THREAD_RUNNING,
		THREAD_STOPPING,
		THREAD_STOPPED,
	} status;
	bool             halt;
};

/** A database of all the open sockets in the system. */
struct sock_db {
	/** The file descriptor for this socket */
	int fd;
	/** A custom context for this socket */
	void *ctx;
	/** Function for freeing the context */
	httpd_free_sess_ctx_fn_t free_ctx;
	/** Send function for this socket */
	httpd_send_func_t send_fn;
	/** Send function for this socket */
	httpd_recv_func_t recv_fn;
};

struct httpd_req_aux {
	struct sock_db  *sd;
	/* Temporary buffer for our operations. Allocate dynamically? */
	char             scratch[HTTPD_SCRATCH_BUF];
	/* Amount of data remaining to be fetched */
	size_t           remaining_len;
	/* HTTP response's status code */
	char            *status;
	/* HTTP response's content type */
	char            *content_type;
};

struct httpd_data {
	/* Information for the HTTPd thread */
	struct thread_data   hd_td;
	/* The socket database */
	struct sock_db       hd_sd[HTTPD_MAX_OPEN_SOCKETS];
	/* Registered URI handlers */
	struct httpd_uri    *hd_calls[HTTPD_MAX_URI_HANDLERS];
	/* The current HTTPD request */
	struct httpd_req     hd_req;
	/* Additional data about the HTTPD request. This could
	 * potentially be merged with the httpd_req above. But that
	 * is exposed in the API. We could reconsider whether the
	 * httpd_req should be visible to the user, or could we make
	 * it opaque.  */
	struct httpd_req_aux hd_req_aux;
};
extern struct httpd_data hd;

/******************* Session Management ********************/
void httpd_sess_init();
int httpd_sess_new(int newfd);
int httpd_sess_process(int newfd);
void httpd_sess_delete(int fd);
void httpd_sess_set_descriptors(fd_set *fdset, int *maxfd);
int httpd_sess_iterate(int start);

/****************** URI handling ********************/
int httpd_uri(httpd_req_t *req);

/****************** Parsing ********************/
int httpd_parse_hdrs(httpd_req_t *r, struct httpd_req_aux *ra);

int httpd_req_new(httpd_req_t *r, struct sock_db *sd);
int httpd_req_delete(httpd_req_t *r);


/****************** Send/Receive ********************/

/* The HTTPD send/recv functions that should be used for sending out
 * data on the socket.
 *
 */
int httpd_send(httpd_req_t *r, const char *buf, unsigned buf_len);
int httpd_recv(httpd_req_t *r, char *buf, unsigned buf_len);

/* These are the lower level default send/recv function of the
 * HTTPd. These should NEVER be directly called. The semantics of
 * these is exactly similar to send()/recv() of the BSD socket API.
 */
int __httpd_send(int sockfd, const char *buf, unsigned buf_len, int flags);
int __httpd_recv(int sockfd, char *buf, unsigned buf_len, int flags);


#endif /* ! _HTTPD_PRIV_H_ */
