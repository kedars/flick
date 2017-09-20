#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>

#include <httpd.h>

#include "httpd_priv.h"


int httpd_sess_new(int newfd)
{
	httpd_d("new session %d\n", newfd);
	int i;
	for (i = 0; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
		//		httpd_d("db [%d] = %d\n", i, hd.hd_sd[i].fd);
		if (hd.hd_sd[i].fd == -1) {
			memset(&hd.hd_sd[i], 0, sizeof(hd.hd_sd[i]));
			hd.hd_sd[i].fd = newfd;
			hd.hd_sd[i].send_fn = __httpd_send;
			hd.hd_sd[i].recv_fn = __httpd_recv;
			return 0;
		}
	}
	return -OS_FAIL;
}

struct sock_db *httpd_sess_get(int newfd)
{
	int i;
	for (i = 0; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
		if (hd.hd_sd[i].fd == newfd) {
			return &hd.hd_sd[i];
		}
	}
	return NULL;
}

void *httpd_sess_get_ctx(int sockfd)
{
	struct sock_db *sd = httpd_sess_get(sockfd);
	if (sd)
		return sd->ctx;
	else
		return NULL;
}

void httpd_sess_set_descriptors(fd_set *fdset, int *maxfd)
{
	int i;
	*maxfd = -1;
	for (i = 0; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
		if (hd.hd_sd[i].fd != -1) {
			FD_SET(hd.hd_sd[i].fd, fdset);
/*                      httpd_d("FD_SET %d\n", hd.hd_sd[i].fd); */
			if (hd.hd_sd[i].fd > *maxfd) {
				*maxfd = hd.hd_sd[i].fd;
			}
		}
	}
}

void httpd_sess_delete(int fd)
{
	httpd_d("delete session %d\n", fd);
	int i;
	for (i = 0; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
		if (hd.hd_sd[i].fd == fd) {
			hd.hd_sd[i].fd = -1;
			if (hd.hd_sd[i].ctx) {
				if (hd.hd_sd[i].free_ctx)
					hd.hd_sd[i].free_ctx(hd.hd_sd[i].ctx);
				else
					free(hd.hd_sd[i].ctx);
				hd.hd_sd[i].ctx = NULL;
				hd.hd_sd[i].free_ctx = NULL;
			}
		}
	}
}

void httpd_sess_init()
{
	int i;
	for (i = 0; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
		hd.hd_sd[i].fd = -1;
		hd.hd_sd[i].ctx = NULL;
	}
}

void shutdown_handle(void *arg)
{
	httpd_stop();
	othread_delete();
}

/* This MUST return OS_SUCCESS on successful execution. If any other
 * value is returned, everything related to this socket will be
 * cleaned up and the socket will be closed.
 */
int httpd_sess_process(int newfd)
{
	struct sock_db *sd = httpd_sess_get(newfd);
	if (! sd)
		return -OS_FAIL;

	if (httpd_req_new(&hd.hd_req, sd) != OS_SUCCESS)
		return -OS_FAIL;
	if (httpd_uri(&hd.hd_req) < 0)
		return -OS_FAIL;
	if (httpd_req_delete(&hd.hd_req) != OS_SUCCESS)
		return -OS_FAIL;
	return OS_SUCCESS;
}

int httpd_sess_iterate(int start_fd)
{
	int start_index = 0;
	int i;

	if (start_fd != -1) {
		/* Take our index to where this fd is stored */
		for (i = 0; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
			if (hd.hd_sd[i].fd == start_fd) {
				start_index = i + 1;
				break;
			}
		}
	}

	for (i = start_index; i < HTTPD_MAX_OPEN_SOCKETS; i++) {
		if (hd.hd_sd[i].fd != -1) {
			return hd.hd_sd[i].fd;
		}
	}
	return -1;
}

