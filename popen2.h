#ifndef __POPEN2_H_
#define __POPEN2_H_

#include <stdio.h>

FILE* popen2(const char *command, const char *type, int* child_pid);
int pclose2(FILE* fp, int pid);

#endif /* __POPEN2_H_ */
