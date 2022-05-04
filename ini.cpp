#include "ini.h"

#include "trim.h"

#include <string.h>

#define MAX_LINE_LEN     (1024)
#define MAX_SECTION_LEN  (64)
#define MAX_NAME_LEN     (64)

int parse_ini(FILE* file, fnIniCB cb)
{
    char line[MAX_LINE_LEN];

    char section[MAX_SECTION_LEN] = { 0, };
    char prev_name[MAX_NAME_LEN]  = { 0, };

    char* start;
    char* end;
    char* name;
    char* value;

    int lineno = 0;
    int error  = 0;
    const char* errmsg = NULL;

    while (fgets(line, MAX_LINE_LEN, file) != NULL) 
    {
        lineno++;

        start = line;
        start = ltrim(rtrim(start));

        if(*start == ';' || *start == '#') 
            continue;

        if(*start == '[')
        {
            end = strchr(start + 1, ']');
            if(*end == ']') 
            {
                *end = '\0';
                strncpy(section, start + 1, MAX_SECTION_LEN);
                section[MAX_SECTION_LEN - 1] = 0;

                prev_name[0] = '\0';
            }
            else if(!error) 
            {
                error  = lineno;
                errmsg = "missing \']\'";
            }
        }
        else if(*start) 
        {
            end = strchr(start, '=');
            if(!end) end = strchr(start, ':');

            if(end) 
            {
                *end = '\0';
                name = rtrim(start);
                value = ltrim(end + 1);

                // remove comment
                end = strchr(value, '#');
                if(!end) end = strchr(value, ';');
                if(end) *end = '\0';
                rtrim(value);

                strncpy(prev_name, name, MAX_NAME_LEN);
                prev_name[MAX_NAME_LEN -1] = 0;

                if(cb(section, name, value) < 0)
                {
                    error = lineno;
                    errmsg = "handler is failed.";
                }
            }
            else if(!error) 
            {
                error = lineno;
                errmsg = "cannot find \'=:\' for value.";
            }
        }

        if(error)
        {
            fprintf(stderr, "Parsing is failed ! line : %d  reason : %s\n", lineno, errmsg);
            break;
        }
    }

    return error;
}
