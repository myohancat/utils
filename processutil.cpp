/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "ProcessUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include "Log.h"

namespace ProcessUtil
{

static void close_all_fds()
{
    int ii = 0;
    int flags =0;

    for(ii = 3; ii <= getdtablesize(); ii++)
    {
        if((flags = fcntl(ii, F_GETFD)) != -1)
        {
            fcntl(ii, F_SETFD, flags | FD_CLOEXEC);
        }
    }
}

static void init_signal()
{
    sigset_t set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

int get_pid(const char* process)
{
    int nPid = -1;
    char szPid[16] = {0,};
    char szCmd[128] = {0,};
    FILE *pFP = NULL;

    snprintf(szCmd, sizeof(szCmd), "pidof %s", process);
    if((pFP = ::popen(szCmd, "r")) != NULL)
    {
        if(fgets(szPid, sizeof(szPid), pFP) != NULL)
        {
            nPid = atoi(szPid);
        }

        ::pclose(pFP);
    }

    return nPid;
}

static int _which_number(const char* s)
{
    for (const char* p = s; *p; p++)
    {
        if (*p < '0' || *p > '9')
            return -1;
    }

    return atoi(s);
}

int get_pid_from_proc_by_name(const char* process)
{
    DIR *dp;
    struct dirent *dir;
    char buf[100], line[1024], tag[100], name[100];
    int pid;
    FILE *fp;

    dp = opendir("/proc");
    if (!dp)
        return -1;

    while ((dir = readdir(dp)))
    {
        pid = _which_number(dir->d_name);

        if (pid == -1)
            continue;

        snprintf(buf, 100, "/proc/%d/status", pid);
        fp = fopen(buf, "r");
        if (fp == NULL)
            continue;

        fgets(line, 1024, fp);
        fclose(fp);

        sscanf(line, "%s %s", tag, name);
        if (!strcmp (name, process))
        {
            closedir(dp);
            return pid;
        }
    }

    closedir(dp);
    return -1;
}

int kill(const char* process)
{
    int pid = get_pid(process);

    if(pid < 0)
    {
        LOGE("cannot found %s : pid %d", process, pid);
        return -1;
    }

    ::kill(pid, SIGTERM);

    return 0;
}

int kill_force(const char* process)
{
    int pid = get_pid(process);
    if(pid < 0)
    {
        LOGE("cannot found %s : pid %d", process, pid);
        return -1;
    }

    ::kill(pid, SIGKILL);

    return 0;
}

static bool _is_exist_process(int pid)
{
    char path[1024];
    sprintf(path, "/proc/%d", pid);
    if (::access(path, F_OK) == 0)
        return true;

    return false;
}

#define POLL_TIME 100
int kill_wait(const char* process, int timeoutMs)
{
    int pid = get_pid(process);
    if (pid < 0)
        return 0;

    ::kill(pid, SIGTERM);

    int retry = timeoutMs / POLL_TIME;
    int remain = timeoutMs - (retry * POLL_TIME);
    for (int ii = 0; ii < retry; ii++)
    {
        usleep(POLL_TIME * 1000);
        if (!_is_exist_process(pid))
            return 0;
    }
    if (remain > 0)
    {
        usleep(remain * 1000);
        if (!_is_exist_process(pid))
            return 0;
    }

    return -1;
}

int system(const char* command)
{
    pid_t pid;
    int   status;

    close_all_fds();

    pid = fork();
    if(pid < (pid_t)0)
    {
        LOGE("Fork is failed !");
        return -1;
    }

    if(pid == (pid_t)0) // CHILD
    {
        const char* new_argv[4];
        new_argv[0] = "sh";
        new_argv[1] = "-c";
        new_argv[2] = command;
        new_argv[3] = NULL;

        init_signal();

        execve("/bin/sh", (char *const *) new_argv, __environ);
        _exit(127);
    }
    else // Parent
    {
        pid_t ret;
        do
        {
            ret = waitpid(pid, &status, 0);
        }while(ret == (pid_t)-1 && errno == EINTR);
        if(ret != pid)
        {
            LOGE("Wait PID is failed !");
            status = -1;
        }
    }

    return status;
}

#define MAX_CMDLINE    (4*1024)
static char** make_args(const char* command)
{
    char   buf[MAX_CMDLINE];
    char** argv = NULL;
    int    argc = 0;
    char*  p = buf, *start = NULL, *end = NULL;

    if (command == NULL)
        return NULL;

    strncpy(buf, command, MAX_CMDLINE -1);
    buf[MAX_CMDLINE -1] = 0;

    while(*p)
    {
        if (!start)
        {
            while(IS_SPACE(*p)) p++;
            if (*p == 0) break;
            start = p;
        }
        else
        {
            if (!IS_QUOTE(*start))
            {
                while(*p && !IS_SPACE(*p)) p++;
                if (*p == 0) end = --p;
                else { end = p - 1; *p = 0; }
            }
            else
            {
                while(*p && *start != *p) p++;
                if (*p == 0) end = --p;
                else { start ++; end = p - 1; *p = 0; }
            }
        }

        if (start && end)
        {
            argv = (char**)realloc(argv, sizeof(char*) * (argc + 1));
            argv[argc++] = strdup(start);
            start = end = NULL;
        }

        p++;
    }

    if (argv)
    {
        argv = (char**)realloc(argv, sizeof(char*) * (argc + 1));
        argv[argc] = NULL;
    }

    return argv;
}

static void free_args(char** argv)
{
    int ii;
    if (!argv)
        return;

    for(ii = 0; argv[ii]; ii++)
        free(argv[ii]);

    free(argv);
}

FILE* popen2(const char *command, const char *type, int* child_pid)
{
    int fds[2];
    pid_t pid;

    if (!command)
    {
        LOGE("Command is NULL");
        return NULL;
    }

    close_all_fds();

    if (pipe(fds) != 0)
    {
        LOGE("create pipe failed. errno : %d", errno);
        return NULL;
    }

    pid = fork();
    if (pid == -1)
    {
        LOGE("create child failed. errno : %d", errno);
        return NULL;
    }

    if (pid == 0) /* Child Process */
    {
        char** argv = make_args(command);
        if (strcmp(type, "r") == 0)
        {
            close(fds[0]);
            dup2(fds[1], 1); /* STDOUT -> PIPE[1] */
        }
        else
        {
            close(fds[1]);
            dup2(fds[0], 0); /* STDIN -> PIPE[0] */
        }

        setpgid(pid, pid);
        execvp(argv[0], argv);

        UNUSED(free_args);

        exit(0);
    }

    *child_pid = pid;
    /* Parent Process */
    if (strcmp(type, "r") == 0)
    {
        close(fds[1]);
        return fdopen(fds[0], "r");
    }
    else
    {
        close(fds[0]);
        return fdopen(fds[1], "w");
    }
}

int pclose2(FILE* fp, int pid)
{
    int stat;

    fclose(fp);

    while (waitpid(pid, &stat, 0) == -1)
    {
        if (errno != EINTR)
        {
            stat = -1;
            break;
        }
    }

    return stat;
}

} // namepspace ProcessUtil
