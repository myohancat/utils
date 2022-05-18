#include "trim.h"

#include <stdio.h>
#include <string.h>

#define ISSPACE(c)     ((c)==' ' || (c)=='\f' || (c)=='\n' || (c)=='\r' || (c)=='\t' || (c)=='\v')

char* ltrim(char *s) 
{
    if(!s) return s;

    while(ISSPACE(*s)) s++;

    return s;
}

char* rtrim(char* s) 
{
    char* back;

    if(!s) return s;

    back = s + strlen(s);

    while((s <= --back) && ISSPACE(*back));
    *(back + 1)    = '\0';

    return s;
}

#define TEST_MAIN
#ifdef TEST_MAIN
int main(int argc, char* arv[])
{
    char testStr[] = "     HaHa   HeHe   ";

    printf("Orginal : \"%s\"\n", testStr);
    printf("ltrim   : \"%s\"\n", ltrim(testStr));
    printf("rtrim   : \"%s\"\n", rtrim(testStr));
    printf("trim    : \"%s\"\n", trim(testStr));

    return 0;
}
#endif
