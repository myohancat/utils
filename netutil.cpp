#include "netutil.h"

#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "log.h"

namespace NetUtil
{

int poll2(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int ret = 0;
    sigset_t org_mask;
    sigset_t new_mask;

    memset(&org_mask, 0, sizeof(org_mask));
    memset(&new_mask, 0, sizeof(new_mask));

    sigaddset(&new_mask, SIGHUP);
    sigaddset(&new_mask, SIGALRM);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGUSR2);

    sigprocmask(SIG_SETMASK, &new_mask, &org_mask);
    ret = poll(fds, nfds, timeout);
    sigprocmask(SIG_SETMASK, &org_mask, NULL);

    return ret;
}

int fd_poll(int fd, int req, int timeout, int fd_int)
{
    int ret = 0;
    struct pollfd fds[2];
    nfds_t nfds = 1;

    fds[0].fd = fd;
    fds[0].events = (short)req | POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;
    fds[0].revents = 0;

    if (fd_int >= 0)
    {
        fds[1].fd = fd_int;
        fds[1].events = POLLIN;
        fds[1].revents = 0;
        nfds = 2;
    }
            
    ret = poll2(fds, nfds, timeout);
    if ( ret == 0 )
    {
        LOG_ERROR("poll timeout - %d msec. GIVE UP.\n", timeout);
        return POLL_TIMEOUT;    /* poll timeout */
    }

    if ( ret < 0 )
    {
        LOG_ERROR("poll failed. ret=%d, errno=%d.\n", ret, errno);
        return -errno;    /* poll failed */
    }

    if (nfds == 2 && fds[1].revents & POLLIN)
    {
        /* W/A code for flush pipe */
        LOG_WARN("!!!! INTERRUPTED !!!!\n");
        char buf[32];
        (void) read(fd_int, buf, sizeof(buf));
        return POLL_INTERRUPTED;
    }

    if ( fds[0].revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL) )
    {
        if ( fds[0].revents & POLLRDHUP )
            LOG_ERROR("POLLRDHUP.\n");
        if ( fds[0].revents & POLLERR )
            LOG_ERROR("POLLERR.\n");
        if ( fds[0].revents & POLLHUP )
            LOG_ERROR("POLLHUP.\n");
        if ( fds[0].revents & POLLNVAL )
            LOG_ERROR("POLLNVAL.\n");

        return -1;    /* fd error detected */
    }

    if (fds[0].revents & req)
    {
        return POLL_SUCCESS;
    }

    LOG_ERROR("XXXXXXXX What Case XXXXXXX\n");
    return -1;
}

int connect2(int sock, const char* ipaddr, int port, int timeout, int fd_int)
{
    int ret;
    struct sockaddr_in serv_addr;
    struct pollfd pfd;

    socket_set_blocking(sock, false);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    LOG_INFO("connect to %s:%d\n", ipaddr, port);

    if (inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr) <= 0)
    {
        LOG_ERROR("Invalid address/ Address not supported \n");
        return -1;
    }

    ret = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (errno > 0 && errno != EAGAIN && errno != EINPROGRESS)
    {
        LOG_ERROR("Cannot connect !, errno = %d\n", errno);
        ret = -1;
    }

    ret = fd_poll(sock, POLLOUT, timeout, fd_int);
    if (ret == POLL_SUCCESS)
    {
        LOG_INFO("Connection is established.\n");
        ret = 0;
    }
    else if (ret == POLL_TIMEOUT) // TIMEOUT
    {
        LOG_ERROR("Connecton timeout ... !\n");
        ret = -1;
    }
    else if (ret == POLL_INTERRUPTED)
    {
        LOG_ERROR("!!! INTERUPTED !!!!\n");
        ret = -1;
    }
    else
    {
        LOG_ERROR("Poll error : %d\n", ret);
        ret = -1;
    }

END:
    socket_set_blocking(sock, true);

    return ret;
}


int bind2(int sock, const char* ipaddr, int port, bool multicast)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons (port);

    if (!ipaddr 
      || strcmp(ipaddr, "127.0.0.1") == 0
      || strcmp(ipaddr, "localhost") == 0)
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, ipaddr, &addr.sin_addr) <= 0)
        {
            LOG_ERROR("Invalid address. %s\n", ipaddr);
            return -1;
        }
    }

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        LOG_ERROR("bind failed. errno : %d(%s)\n", errno, strerror(errno));
        return -1;
    }
    
    return 0;
}


int socket_set_blocking(int sock, bool block)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if(block)
        flags = flags & (O_NONBLOCK ^ 0xFFFFFFFF);
    else
        flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        LOG_ERROR("Cannot set socket blocking\n");
        return -1;
    }

    return 0;
}


int socket_set_reuseaddr(int sock)
{
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
    {
        LOG_ERROR("Cannot set reuseaddr failed. \n");
        return -1;
    }

    return 0;
}


int socket_set_buffer_size(int sock, int rcvBufSize, int sndBufSize)
{
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(rcvBufSize)) != 0)
    {
        LOG_WARN("Cannot set Receive Buffer size \n");
    }

    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize)) != 0)
    {
        LOG_WARN("Cannot set Receive Buffer size \n");
    }

    return 0;
}


} // namespace NetUtil
