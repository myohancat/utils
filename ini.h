#ifndef __INI_H_
#define __INI_H_

#include <stdio.h>

typedef int (*fnIniCB)(const char* section, const char* name, const char* value);
int parse_ini(FILE* file, fnIniCB cb);

#endif /* __INI_H_ */
