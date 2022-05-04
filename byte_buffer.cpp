#include "byte_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

typedef struct ByteBuffer_s 
{
	bool mEOS;
	int  mCapacity;
	int  mSize;

	int  mFront;
	int  mRear;

	byte* mBuffer;

	pthread_mutex_t mLock;
	pthread_cond_t  mCondVarFull;
	pthread_cond_t  mCondVarEmpty;

}ByteBuffer_t;

#define AVAILABLE_SPACE(buffer)		(buffer->mCapacity - buffer->mSize)

#ifndef MIN
#define MIN(x, y)		(x > y)?(y):(x)
#endif

ByteBuffer ByteBuffer_Create(int capacity) 
{
	int rc;
	pthread_condattr_t attr;
	
	ByteBuffer buffer = (ByteBuffer)malloc(sizeof(ByteBuffer_t) + sizeof(byte) * capacity);
	if(buffer == NULL)
	{
		printf("Cannot allocate byte buffer !!\n");
		goto ERROR;
	}
	
	if((rc = pthread_mutex_init(&buffer->mLock, NULL)) != 0)
	{
		printf("Cannot init mutext !!\n");
		goto ERROR;
	}

	if((rc = pthread_condattr_init(&attr)) != 0)
	{
		printf("Cannot init cond attribute !!\n");
		pthread_mutex_destroy(&buffer->mLock);
	 	goto ERROR;
	}

	if((rc = pthread_cond_init(&buffer->mCondVarFull, &attr)) != 0)
	{
		printf("Cannot init condvariable Full !!\n");
		pthread_mutex_destroy(&buffer->mLock);
		goto ERROR;
	}

	if((rc = pthread_cond_init(&buffer->mCondVarEmpty, &attr)) != 0)
	{
		printf("Cannot init condvariable Empty !!\n");
		pthread_mutex_destroy(&buffer->mLock);
		goto ERROR;
	}
	
	buffer->mEOS      = 0;
	buffer->mCapacity = capacity;
	buffer->mSize     = 0;
	buffer->mFront    = 0;
	buffer->mRear     = 0;
	buffer->mBuffer   = (byte*)(buffer + 1);

	goto EXIT;
ERROR:
	if(buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}
EXIT:

	return buffer;
}

void ByteBuffer_Delete(ByteBuffer buffer)
{
	if(!buffer)
		return;

	pthread_mutex_destroy(&buffer->mLock);
	pthread_cond_destroy(&buffer->mCondVarFull);
	pthread_cond_destroy(&buffer->mCondVarEmpty);

	free(buffer);
} 

int ByteBuffer_Write(ByteBuffer buffer, const byte* data, int len, int timeout)
{
	int rc;
	int writeSize;

	if(buffer == NULL)
	{
		return BUFFER_ERROR;
	}

	if(data == NULL || len < 1)
	{
		printf("data is invalid data : %p len : %d\n", data, len);
		return BUFFER_ERROR;
	}

	pthread_mutex_lock(&buffer->mLock);

	if(AVAILABLE_SPACE(buffer) == 0 && !buffer->mEOS)
	{
		if(timeout == 0)
		{
			pthread_mutex_unlock(&buffer->mLock);
			return 0;
		}
		
		if(timeout == -1)
		{
			rc = pthread_cond_wait(&buffer->mCondVarFull, &buffer->mLock);
		}
		else
		{
			struct timespec target;

			struct timeval now;
			rc = gettimeofday(&now, NULL);
			target.tv_nsec = now.tv_usec * 1000 + (timeout%1000)*1000000;
			target.tv_sec = now.tv_sec + (timeout/1000);

			if(target.tv_nsec > 1000000000) 
			{
				target.tv_nsec -=  1000000000;
				target.tv_sec ++;
			}
	
			rc = pthread_cond_timedwait(&buffer->mCondVarFull, &buffer->mLock, &target);
			if(rc == ETIMEDOUT) 
			{
				pthread_mutex_unlock(&buffer->mLock);
				return BUFFER_ERROR_TIMEOUT;
			}
		}
	}

	if(buffer->mEOS)
	{
		printf("cannot write data, EOS occured.. \n");
		pthread_mutex_unlock(&buffer->mLock);
		return BUFFER_ERROR_EOS;
	}

	if(len > AVAILABLE_SPACE(buffer))
		len = AVAILABLE_SPACE(buffer);

	writeSize = buffer->mCapacity - buffer->mRear;
	if(writeSize < len)
	{
		memcpy(&buffer->mBuffer[buffer->mRear], data, writeSize);
		memcpy(&buffer->mBuffer[0], data + writeSize, len - writeSize);
		buffer->mRear = len - writeSize;
	}
	else
	{
		memcpy(&buffer->mBuffer[buffer->mRear], data, len);
		buffer->mRear += len;
	}

	buffer->mSize += len;

	pthread_cond_signal(&buffer->mCondVarEmpty);

	pthread_mutex_unlock(&buffer->mLock);

	return len;
}

int ByteBuffer_Read(ByteBuffer buffer, byte* data, int len, int timeout) 
{
	int rc;
	int readSize;

	if(buffer == NULL)
		return BUFFER_ERROR;

	if(data == NULL || len < 1)
		return BUFFER_ERROR;

	pthread_mutex_lock(&buffer->mLock);

	if(buffer->mSize == 0 || !buffer->mEOS)
	{
		if(timeout == 0)
		{
			pthread_mutex_unlock(&buffer->mLock);
			return 0;
		}
		
		if(timeout == -1)
		{
			rc = pthread_cond_wait(&buffer->mCondVarEmpty, &buffer->mLock);
		}
		else
		{
			struct timespec target;

			struct timeval now;
			rc = gettimeofday(&now, NULL);
			target.tv_nsec = now.tv_usec * 1000 + (timeout%1000)*1000000;
			target.tv_sec = now.tv_sec + (timeout/1000);

			if(target.tv_nsec > 1000000000) 
			{
				target.tv_nsec -=  1000000000;
				target.tv_sec ++;
			}
	
			rc = pthread_cond_timedwait(&buffer->mCondVarEmpty, &buffer->mLock, &target);
			if(rc == ETIMEDOUT) 
			{
				pthread_mutex_unlock(&buffer->mLock);
				return BUFFER_ERROR_TIMEOUT;
			}
		}
	}

	if(buffer->mEOS)
	{
		printf("cannot read data, EOS occured.. \n");
		pthread_mutex_unlock(&buffer->mLock);
		return BUFFER_ERROR_EOS;
	}

	if(buffer->mSize < len)
		len = buffer->mSize;

	readSize = buffer->mCapacity - buffer->mFront;
	if(readSize < len)
	{
		memcpy(data, &buffer->mBuffer[buffer->mFront], readSize);
		memcpy(data + readSize, &buffer->mBuffer[0], len - readSize);
		buffer->mFront = len - readSize;
	}
	else
	{
		memcpy(data, &buffer->mBuffer[buffer->mFront], len);
		buffer->mFront += len;
	}

	buffer->mSize -= len;

	pthread_cond_signal(&buffer->mCondVarFull);

	pthread_mutex_unlock(&buffer->mLock);

	return len;
}

int ByteBuffer_AvailableData(ByteBuffer buffer)
{
	int size;

	if(buffer == NULL)
		return -1;

	pthread_mutex_lock(&buffer->mLock);
	size = buffer->mSize;
	pthread_mutex_unlock(&buffer->mLock);

	return size;
}

int ByteBuffer_AvailableSpace(ByteBuffer buffer)
{
	int size;

	if(buffer == NULL)
		return -1;

	pthread_mutex_lock(&buffer->mLock);
	size = buffer->mCapacity - buffer->mSize;
	pthread_mutex_unlock(&buffer->mLock);

	return size;
}

void ByteBuffer_SetEOS(ByteBuffer buffer, bool isEOS)
{
	if(buffer == NULL)
        return;

	pthread_mutex_lock(&buffer->mLock);

	buffer->mEOS = isEOS;

	pthread_cond_signal(&buffer->mCondVarFull);
	pthread_cond_signal(&buffer->mCondVarEmpty);

	pthread_mutex_unlock(&buffer->mLock);	
}

int ByteBuffer_Flush(ByteBuffer buffer)
{
	if(buffer == NULL)
		return BUFFER_ERROR;

	pthread_mutex_lock(&buffer->mLock);

	buffer->mSize  = 0;
	buffer->mFront = 0;
	buffer->mRear  = 0;

	pthread_mutex_unlock(&buffer->mLock);
	
	return 0;
}

