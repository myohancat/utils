#include "parcel.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "log.h"

#define DEFAULT_PARCEL_DATA_SIZE     (256)

Parcel::Parcel()
       : mData(NULL),
         mDataPos(0),
         mDataSize(0),
         mDataCapacity(0)
{
}

Parcel::~Parcel()
{
    if(mData != NULL)
        free(mData);
}

int Parcel::send(int fd, Parcel& parcel)
{
    ssize_t wsize;

    parcel.rewind();

    wsize = write(fd, &parcel.mDataSize, 4);
    if(wsize < 0)
        return -1;

    wsize = write(fd, parcel.mData, parcel.mDataSize);
    if(wsize < 0)
        return -1;
    
    return 0;
}

int Parcel::recv(int fd, Parcel& parcel)
{
    size_t totalRead = 0;
    ssize_t rsize = 0;

    parcel.reset();

    rsize = read(fd, &parcel.mDataSize, 4);
    if(rsize < 0)
        return -1;

    parcel.mData = (uint8_t*)malloc(parcel.mDataSize);
    if(!parcel.mData)
        return -1;

    while(parcel.mDataSize > totalRead)
    {
        rsize = read(fd, (void*)(parcel.mData + totalRead), parcel.mDataSize - totalRead);
        if(rsize < 0)
        {
            parcel.reset();
            return -1;
        }
        totalRead += rsize;
    }

    return 0;
}

void Parcel::growData(int size)
{
    uint32_t newSize = ((mDataCapacity+size)*3)/2; 

    // Enhance Performance
    if(newSize < DEFAULT_PARCEL_DATA_SIZE)
        newSize = DEFAULT_PARCEL_DATA_SIZE;

    if(mData == NULL)
        mData = (uint8_t*)malloc(newSize);
    else
        mData = (uint8_t*)realloc(mData, newSize);

    mDataCapacity = newSize;
}

void Parcel::rewind()
{
    mDataPos = 0;
}

void Parcel::reset()
{
    mDataPos  = 0;
    mDataSize = 0;
    mDataCapacity = 0;
    if(mData != NULL)
    {
        free(mData);
        mData = NULL;
    }
}

void Parcel::write8(uint8_t val)
{
    if((mDataPos + sizeof(val)) > mDataCapacity)
        growData(sizeof(val));

    mData[mDataPos] = val;
    mDataPos  += sizeof(val);
    mDataSize += sizeof(val);
}

void Parcel::write16(uint16_t val)
{
    if((mDataPos + sizeof(val)) > mDataCapacity)
        growData(sizeof(val));

    uint16_t* pDataPtr = (uint16_t*) &mData[mDataPos];
    *pDataPtr = val;
    mDataPos  += sizeof(val);
    mDataSize += sizeof(val);
}

void Parcel::write32(uint32_t val)
{
    if((mDataPos + sizeof(val)) > mDataCapacity)
        growData(sizeof(val));

    uint32_t* pDataPtr = (uint32_t*)& mData[mDataPos];
    *pDataPtr = val;
    mDataPos  += sizeof(val);
    mDataSize += sizeof(val);
}

void Parcel::write64(uint64_t val)
{
    if((mDataPos + sizeof(val)) > mDataCapacity)
        growData(sizeof(val));

    uint64_t* pDataPtr = (uint64_t*)& mData[mDataPos];
    *pDataPtr = val;
    mDataPos  += sizeof(val);
    mDataSize += sizeof(val);
}

void Parcel::writeString(const char* str)
{
    int len = strlen(str) + 1;

    if((mDataPos + len) > mDataCapacity)
        growData(len);

    memcpy(&mData[mDataPos], str, len);
    mDataPos  += len;
    mDataSize += len;
}

void Parcel::writeBytes(const void* data, int len)
{
    if((mDataPos + len) > mDataCapacity)
        growData(len);

    memcpy(&mData[mDataPos], data, len);
    mDataPos  += len;
    mDataSize += len;
}

uint8_t Parcel::read8()
{
    uint8_t val;
    int len = sizeof(val);

    if((mDataPos + len) > mDataSize)
        return 0;

    val = mData[mDataPos];
    mDataPos += len;
    
    return val;
}

uint16_t Parcel::read16()
{
    uint16_t val;
    int len = sizeof(val);

    if((mDataPos + len) > mDataSize)
        return 0;

    uint16_t* pData = (uint16_t*)&mData[mDataPos];
    val = *pData;
    mDataPos += len;
    
    return val;
}


uint32_t Parcel::read32()
{
    uint32_t val;
    int len = sizeof(val);

    if((mDataPos + len) > mDataSize)
        return -1;

    uint32_t* pData = (uint32_t*)&mData[mDataPos];
    val = *pData;
    mDataPos += len;
    
    return val;
}

uint64_t Parcel::read64()
{
    uint64_t val;
    int len = sizeof(val);

    if((mDataPos + len) > mDataSize)
        return -1;

    uint64_t* pData = (uint64_t*)&mData[mDataPos];
    val = *pData;
    mDataPos += len;
    
    return val;
}

const char* Parcel::readString()
{
    const char* pData = (const char*)&mData[mDataPos];
    int len = strlen(pData) + 1;

    if((mDataPos + len) > mDataSize)
        return NULL;

    mDataPos += len;
    
    return pData;
}

uint8_t* Parcel::readBytes(void* data, int len)
{
    uint8_t* pData = (uint8_t*)&mData[mDataPos];
    
    if((mDataPos + len) > mDataSize)
        return NULL;

    if(data != NULL)
    {
        memcpy(data, pData, len);
        pData = (uint8_t*)data;
    }

    mDataPos += len;
    
    return pData;
}
