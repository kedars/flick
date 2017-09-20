/**
 * This is the web server.
 *
 * Requirements:
 * - Single-threaded web server that can serve one client at a time
 * - Should support HTTP pipelining (multiple requests on the same
 *   socket)
 * - Should support multiple open connections at the same time (only
 *   one is actively served, while the other connections are open)
 * - Should allow preservation of context across multiple requests on
 *   the same socket
 * - Should allow overriding of the web server's send/receive function
 *   on a per-socket basis
 * - Not expected to support HTTPS (443) as of now
 *
 */

#include <ctrl_sock.h>
#include <httpd.h>
#include <string.h>
#include "httpd_priv.h"

#define HTTPD_STACK_SIZE   (12 * 1024)
#define HTTPD_PORT         80
#define HTTPD_BACKLOG      5
#define HTTPD_CTRL_SOCK_PORT  54321

struct httpd_data hd;

static void httpd_accept_conn(int listen_fd)
{
	struct sockaddr_in addr_from;
	socklen_t addr_from_len = sizeof(addr_from);
	int new_fd = accept(listen_fd, (struct sockaddr *)&addr_from, &addr_from_len);
	if (new_fd < 0) {
		httpd_d("Error in accept, what to do?\n");
		return;
	}
	httpd_d("accept_conn: newfd = %d\n", new_fd);

	if (httpd_sess_new(new_fd)) {
		httpd_d("Warn: No more space for new sessions\n");
		close(new_fd);
	}
	httpd_d("after sess_new\n");
	return;
}


/* Manage in-coming connection or data requests */
static void httpd_server(int listen_fd, int ctrl_fd)
{
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(listen_fd, &read_set);
	FD_SET(ctrl_fd, &read_set);

	int tmp_max_fd;
	httpd_sess_set_descriptors(&read_set, &tmp_max_fd);
	int maxfd = (listen_fd > tmp_max_fd) ? listen_fd : tmp_max_fd;
	tmp_max_fd = maxfd;
	maxfd = (ctrl_fd > tmp_max_fd) ? ctrl_fd : tmp_max_fd;

	//       	httpd_d("doing select maxfd+1 = %d\n", maxfd +1);
	int active_cnt = select(maxfd + 1, &read_set, NULL, NULL, NULL);
	if (active_cnt < 0) {
		httpd_d("Error in select, what to do? %d\n", active_cnt);
		return;
	}

	/* Case0: Do we have a control message? */
	if (FD_ISSET(ctrl_fd, &read_set)) {
		/* For now, the only message is shutdown, so don't
		 * even read it */
		int fd = -1;
		while( (fd = httpd_sess_iterate(fd)) != -1) {
			httpd_d("cleaning up socket %d\n", fd);
			httpd_sess_delete(fd);
			close(fd);
		}
	}

	/* Case1: Do we have any activity on the current data
	 * sessions? */
	int fd = -1;
	while( (fd = httpd_sess_iterate(fd)) != -1) {
		if (FD_ISSET(fd, &read_set)) {
			httpd_d("processing socket %d\n", fd);
			if (httpd_sess_process(fd) != OS_SUCCESS) {
				httpd_d("cleaning up socket %d\n", fd);
				httpd_sess_delete(fd);
				close(fd);
			}
		}
	}

	/* Case2: Do we have any incoming connection requests to
	 * process? */
	if (FD_ISSET(listen_fd, &read_set)) {
		httpd_d("processing listen socket %d\n", listen_fd);
		httpd_accept_conn(listen_fd);
	}
}

/* The main HTTPD thread */
static void httpd_thread(void *arg)
{
	hd.hd_td.status = THREAD_RUNNING;

	int fd;
	fd = socket(PF_INET6, SOCK_STREAM, 0);

	struct sockaddr_in6 serv_addr;
	struct in6_addr inaddr_any = IN6ADDR_ANY_INIT;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin6_family  = PF_INET6;
	serv_addr.sin6_addr    = inaddr_any;
	serv_addr.sin6_port    = htons(HTTPD_PORT);
	int ret = bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret)
	     httpd_d("bind failed: %d\n", ret);

	ret = listen(fd, HTTPD_BACKLOG);
	if (ret)
	     httpd_d("listen failed: %d\n", ret);

	int ctrl_fd = cs_create_ctrl_sock(HTTPD_CTRL_SOCK_PORT);

	httpd_d("Web server started\n");
	while (1) {
		httpd_server(fd, ctrl_fd);

		/* We were asked to be halted, perform cleanup and
		 * exit */
		if (hd.hd_td.halt) {
			hd.hd_td.status = THREAD_STOPPING;
			break;
		}
	}
	httpd_d("Web server exiting\n");
	cs_free_ctrl_sock(ctrl_fd);
	close(fd);
	hd.hd_td.status = THREAD_STOPPED;
	othread_delete();
}

int httpd_start()
{
	int ret;

	ret = othread_create(&hd.hd_td.handle, "httpd", HTTPD_STACK_SIZE,
			     OS_DEFAULT_PRIORITY, httpd_thread, NULL);
	httpd_sess_init();
	return ret;
}

void httpd_stop()
{
	hd.hd_td.halt = true;
	char *halt_cmd = "halt";
 	cs_send_to_ctrl_sock(HTTPD_CTRL_SOCK_PORT, halt_cmd, strlen(halt_cmd));

	/* This isn't the most efficient, eg can use semaphore too,
	 * but should be ok for most cases where the 'stop' is rare.
	 */
	while (hd.hd_td.status != THREAD_STOPPED)
		othread_sleep(1000);

	memset(&hd, 0, sizeof(hd));
}
