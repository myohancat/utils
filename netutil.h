#ifndef __NET_UTIL_H_
#define __NET_UTIL_H_

#include <poll.h>

namespace NetUtil
{

int poll2(struct pollfd *fds, nfds_t nfds, int timeout);

#define POLL_SUCCESS     (1)
#define POLL_INTERRUPTED (2)
#define POLL_TIMEOUT     (0)

#define POLL_REQ_IN      (POLLIN)
#define POLL_REQ_OUT     (POLLOUT)
int fd_poll(int fd, int req, int timeout, int fd_int = -1);

int connect2(int sock, const char* ipaddr, int port, int timeout, int fd_int = -1);
int bind2(int sock, const char* ipaddr, int port, bool multicast = false);

/* Socket Option */
int socket_set_blocking(int sock, bool block);
int socket_set_reuseaddr(int sock);
int socket_set_buffer_size(int sock, int rcvBufSize, int sndBufSize);

} // namespace NetUtil


#endif /* __NET_UTIL_H_ */
