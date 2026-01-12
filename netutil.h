/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __NET_UTIL_H_
#define __NET_UTIL_H_

#include <stdint.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>

typedef enum
{
    SOCK_TYPE_TCP,
    SOCK_TYPE_UDP,

    MAX_SOCK_TYPE
} SockType_e;

typedef struct MacAddr_s
{
    char    str[32];
    uint8_t raw[6];
} MacAddr_t;

typedef struct IpAddr_s
{
    char    str[32];
    uint8_t raw[4];
} IpAddr_t;

namespace NetUtil
{

int socket(SockType_e type);

int poll(struct pollfd *fds, nfds_t nfds, int timeout);

#define POLL_SUCCESS     (1)
#define POLL_INTERRUPTED (2)
#define POLL_TIMEOUT     (0)

#define POLL_REQ_IN      (POLLIN)
#define POLL_REQ_PRI     (POLLPRI | POLLIN)
#define POLL_REQ_OUT     (POLLOUT)
int fd_poll(int fd, int req, int timeout, int fd_int = -1);

/**
 * return  == 0, success
 * return < 0, error occured.
 * especially, timeout error is -ETIMEDOUT
 */
#define NET_ERR_SUCCESS     0
#define NET_ERR_TIMEOUT     -ETIMEDOUT
int connect(int sock, const char* ipaddr, int port, int timeout, int fd_int = -1);
int bind(int sock, const char* ipaddr, int port);
int listen(int sock, int backlog);
int accept(int sock, char* clntaddr, int addrlen);

/**
 * return > 0, success.
 * return < 0, error occured.
 * especially, timeout error is -ETIMEDOUT
 * return == 0, interrupted.
 */
int send(int sock, const void *buf, size_t len, int timeoutMs, int fd_int = -1);
int recv(int sock, void *buf, size_t len, int timeoutMs, int fd_int = -1);
int sendto(int sock, const char* ipaddr, int port, void *buf, size_t len, int timeoutMs, int fd_int = -1);
int recvfrom(int sock, char* ipaddr, int iplen, void *buf, size_t len, int timeoutMs, int fd_int = -1);

/**
 * return == 0, success
 * return < 0, error occured.
 */
/* Socket Option */
int socket_set_blocking(int sock, bool block);
int socket_set_reuseaddr(int sock);

int socket_set_recv_buf_size(int sock, int bufSize);
int socket_get_recv_buf_size(int sock);

int socket_set_send_buf_size(int sock, int bufSize);
int socket_get_send_buf_size(int sock);

/* network information */
int get_link_state(const char* ifname, bool* isUP);
int get_cable_state(const char* ifname, bool* isPugged);
int get_mac_addr(const char* ifname, MacAddr_t* mac);
int get_ip_addr(const char* ifname, char* ip);
int get_netmask(const char* ifname, char* netmask);
int get_default_gateway(char* default_gw, char *interface);

} // namespace NetUtil


#endif /* __NET_UTIL_H_ */
