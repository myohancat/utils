#ifndef __TRIM_H_
#define __TRIM_H_

char* ltrim(char *s);
char* rtrim(char* s);

#define trim(s)        rtrim(ltrim(s))

#endif /*__TRIM_H_*/
