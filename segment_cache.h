#ifndef __SEGMENT_CACHE_H_
#define __SEGMENT_CACHE_H_

#include <inttypes.h>

#ifndef BOOL
typedef int  BOOL;
#endif // BOOL

#ifndef TRUE
#define TRUE 1
#endif // TRUE

#ifndef FALSE
#define FALSE 0
#endif // FALSE

typedef struct SegmentCache_s* SegmentCache;

#define INVALID_INDEX 			(uint32_t)(-1)
SegmentCache SegmentCache_Create(int64_t totalSize, uint32_t segmentSize, uint32_t maxSegmentToCache);
void         SegmentCache_Delete(SegmentCache cache);

BOOL SegmentCache_IsCached(SegmentCache cache, const SegmentInfo_t* info);
int SegmentCache_ReadCache(SegmentCache cache, int64_t offset, uint8_t* data, int len);
int SegmentCache_WriteCache(SegmentCache cache, const SegmentInfo_t* info, const uint8_t* data);

typedef struct SegmentInfo_s
{
	uint32_t mIndex;
	uint32_t mSize;

	int64_t  mOffset;
	uint32_t mBufferd;
}SegmentInfo_t;

int  SegmentCache_GetSegmentInfoByOffset(SegmentCache cache, int64_t offset, SegmentInfo_t* segInfo);
int  SegmentCache_GetSegmentInfoByIndex(SegmentCache cache, uint32_t segmentIndex, SegmentInfo_t* segInfo);

void SegmentCache_Dump(SegmentCache cache);

#endif // __SEGMENT_CACHE_H_
