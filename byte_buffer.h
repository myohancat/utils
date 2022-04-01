#ifndef __BYTE_BUFFER_H_
#define __BYTE_BUFFER_H_

#include <stdbool.h>

typedef unsigned char byte;

typedef struct ByteBuffer_s* ByteBuffer;

ByteBuffer ByteBuffer_Create(int capacity);
void       ByteBuffer_Delete(ByteBuffer buffer);

#define BUFFER_ERROR          (-1)
#define BUFFER_ERROR_TIMEOUT  (-2)
#define BUFFER_ERROR_EOS      (-3)

int ByteBuffer_Write(ByteBuffer buffer, const byte* data, int len, int timeout);
int ByteBuffer_Read(ByteBuffer buffer, byte* data, int len, int timeout);

int ByteBuffer_AvailableData(ByteBuffer buffer);
int ByteBuffer_AvailableSpace(ByteBuffer buffer);

void ByteBuffer_SetEOS(ByteBuffer buffer, bool isEOS);
int  ByteBuffer_Flush(ByteBuffer buffer);

#endif // __HTTP_BUFFER_H_
