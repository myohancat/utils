#include "popen2.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define IS_QUOTE(ch)   (ch == '"' || ch == '\'')
#define MAX_CMDLINE    (4*1024)

static void close_all_fds()
{
    int ii = 0;
    int flags =0;

    for(ii = 0; ii <= getdtablesize(); ii++)
    {
        if((flags = fcntl(ii, F_GETFD)) != -1)
        {
            fcntl(ii, F_SETFD, flags | FD_CLOEXEC);
        }
    }
}

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
            while(isspace(*p)) p++;
            if (*p == 0) break;
            start = p;
        }
        else
        {
            if (!IS_QUOTE(*start))
            {
                while(*p && !isspace(*p)) p++;
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
        fprintf(stderr, "Command is NULL\n");
        return NULL;
    }

    close_all_fds();

    if (pipe(fds) != 0)
    {
        fprintf(stderr, "create pipe failed. errno : %d\n", errno);
        return NULL;
    }

    pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "create child failed. errno : %d\n", errno);
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
