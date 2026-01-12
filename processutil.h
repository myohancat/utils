/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __PROCESS_UTIL_H_
#define __PROCESS_UTIL_H_

#include "Types.h"

namespace ProcessUtil
{

int get_pid(const char* process);
int get_pid_from_proc_by_name(const char* process);

int kill(const char* process);
int kill_force(const char* process);
int kill_wait(const char* process, int timeoutMs);

int system(const char* command);

FILE* popen2(const char *command, const char *type, int* child_pid);
int pclose2(FILE* fp, int pid);

} // namepspace ProcessUtil

#endif /* __PROCESS_UTIL_H_ */
