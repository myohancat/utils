#include "key_value.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


#define MAX_STR_LEN (2*1024)
void parseKeyValueStr(const char* str, fnKeyValueCB cb, void* userdat)
{
    int index = 0;
    char buf[MAX_STR_LEN];
    char* p = buf, *key = NULL, *value = NULL;

    strncpy(buf, str, MAX_STR_LEN);
    buf[MAX_STR_LEN -1] = 0;

    while(*p)
    {
        if (!key)
        {
            while (isspace(*p)) p++;
            if (*p == 0) continue;
            key = p++;
        }
        else if(!value)
        {
            while (*p && *p != '=' && !isspace(*p)) p++;
            if (*p == '=')
            {
                *p++ = 0;
                value = p;
                if(IS_QUOTE(*value))
                {
                    p++;
                    while(*p && *value != *p) p++;
                }
            }
        }

        if (isspace(*p))
        {
            *p++ = 0;
            if (cb)
                cb(index++, key, value, userdat);

            key = value = NULL;
        }
        else
            p++;
    }

    if (key)
    {
        if (cb)
            cb(index++, key, value, userdat);
    }
}
