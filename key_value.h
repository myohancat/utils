#ifndef __KEY_VALUE_H_
#define __KEY_VALUE_H_

#include <stdio.h>

/**
 * Key-Value Parser 
 * example :
 *      key1 key2=value2 key3='value3' key4="value4"
 */
#define IS_QUOTE(ch)   (ch == '"' || ch == '\'')

typedef void (*fnKeyValueCB)(int index, const char* key, const char* value, void* userdat);
void parseKeyValueStr(const char* str, fnKeyValueCB cb, void* userdat);

#endif /* __KEY_VALUE_H_ */
