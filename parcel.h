#ifndef __PARCEL_H_
#define __PARCEL_H_

#include <stdint.h>

class Parcel
{
public:
    static int send(int fd, Parcel& parcel);
    static int recv(int fd, Parcel& parcel);

    Parcel();
    ~Parcel();

    void rewind();
    void reset();

    void write8(uint8_t val);
    void write16(uint16_t val);
    void write32(uint32_t val);
    void write64(uint64_t val);
    void writeBytes(const void* bytes, int len);
    void writeString(const char* str);
    
    uint8_t  read8();
    uint16_t read16();
    uint32_t read32();
    uint64_t read64();
    uint8_t* readBytes(void* bytes, int len);
    const char* readString();

private:
    uint8_t*  mData;
    uint32_t  mDataPos;
    uint32_t  mDataSize;
    uint32_t  mDataCapacity;

    void growData(int size);
};

#endif /* __PARCEL_H_ */
