//#define DBG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "segment_cache.h"

#ifndef CEIL
#define CEIL(x, y)    (1 + (((x) - 1) / y))
#endif

#ifndef MAX
#define MAX(x, y)     ((x)>(y))?(x):(y)
#endif

#ifndef MIN
#define MIN(x, y)      ((x)>(y))?(y):(x)
#endif

typedef unsigned int  Bitmap_t;
#define MAX_BITMAP_VALUE (sizeof(Bitmap_t) * 8)

typedef struct Segment_s
{
    struct Segment_s* mPrev;
    struct Segment_s* mNext;

    uint32_t mIndex;
    uint32_t mSize;

    uint8_t* mData;
}Segment_t;

typedef struct SegmentCache_s
{
    int64_t    mTotalSize;
    uint32_t   mSegmentSize;
    uint32_t   mNumOfSegment;

    uint32_t   mMaxSegmentToCache;

    uint32_t   mNumOfSegBitmap;
    Bitmap_t*  mSegBitmap;

    // Segment Pool
    Segment_t* mPool;
    Segment_t* mFree;
    Segment_t* mAllocHead;
    Segment_t* mAllocTail;

}SegmentCache_t;

#define IS_CACHE_FULL(cache)     (cache->mFree == NULL)
#define IS_CACHE_EMPTY(cache)    (cache->mAllocHead == NULL)

#define GET_BASE_SEGMENT_INDEX(cache, offset)         (uint32_t)((int64_t)offset / cache->mSegmentSize)
#define GET_BASE_SEGMENT_OFFSET(cache, segmentIdx)    (int64_t)((int64_t)segmentIdx * cache->mSegmentSize)
#define IS_LAST_SEGMENT(cache, segmentIdx)            (segmentIdx == (cache->mNumOfSegment -1))
#define GET_LAST_SEGENT_SIZE(cache)                   ((cache->mTotalSize%cache->mSegmentSize == 0)?(cache->mSegmentSize):(cache->mTotalSize%cache->mSegmentSize))
#define GET_SEGMENT_SIZE(cache, segmentIdx)           (uint32_t)(IS_LAST_SEGMENT(cache, segmentIdx)?GET_LAST_SEGENT_SIZE(cache):cache->mSegmentSize)

#define GET_SEGMENT_BITMAP(cache, segmentIdx)         cache->mSegBitmap[segmentIdx / MAX_BITMAP_VALUE]
#define GET_SEGMENT_BITMAP_MASK(cache, segmentIdx)    (uint32_t)(1 << (segmentIdx % MAX_BITMAP_VALUE))

#define IS_SEGMENT_EXIST(cache, segmentIdx)           ((GET_SEGMENT_BITMAP(cache, segmentIdx) & GET_SEGMENT_BITMAP_MASK(cache, segmentIdx)) != 0)
#define SET_SEGMENT_BITMAP(cache, segmentIdx)         (GET_SEGMENT_BITMAP(cache, segmentIdx) |= GET_SEGMENT_BITMAP_MASK(cache, segmentIdx))
#define CLEAR_SEGMENT_BITMAP(cache, segmentIdx)       (GET_SEGMENT_BITMAP(cache, segmentIdx) &=  ~GET_SEGMENT_BITMAP_MASK(cache, segmentIdx))

static int create_segment_pool(SegmentCache cache, uint32_t segmentSize, uint32_t segmentCount)
{
    Segment_t* segment;
    uint32_t ii = 0;
    uint8_t* iter;

    cache->mPool = (Segment_t*)malloc( (sizeof(Segment_t) + sizeof(uint8_t*) * segmentSize) * segmentCount);
    if(cache->mPool == NULL)
    {
        LOG_ERROR("Cannot allocate segment pool\n");
        return -1;
    }
    cache->mFree      = NULL;
    cache->mAllocHead = NULL;
    cache->mAllocTail = NULL;

    iter = (uint8_t*)cache->mPool;

    for(ii = 0; ii < segmentCount; ii++)
    {
        segment = (Segment_t*)iter;
        iter += sizeof(Segment_t);
        memset(segment, 0x00, sizeof(Segment_t));

        segment->mData = iter;
        iter += segmentSize;

        segment->mNext = cache->mFree;
        cache->mFree = segment;
    }

    return 0;
}

static void delete_segment_pool(SegmentCache cache)
{
    cache->mFree      = NULL;
    cache->mAllocHead = NULL;
    cache->mAllocTail  = NULL;

    if(cache->mPool != NULL)
    {
        free(cache->mPool);
        cache->mPool = NULL;
    }
}

static Segment_t* alloc_segment(SegmentCache cache)
{
    Segment_t* segment = cache->mFree;

    if(segment != NULL)
        cache->mFree = segment->mNext;

    return segment;
}

#if 0 // NO NEED TO USE
static void free_segment(SegmentCache cache, Segment_t* segment)
{
    if(segment == NULL)
        return;

    segment->mNext = cache->mFree;
    cache->mFree = segment;
}
#endif

static void insert_alloc_list(SegmentCache cache, Segment_t* segment)
{
    if(segment == NULL)
        return;

    if(cache->mAllocTail == NULL)
    {
        cache->mAllocHead = segment;
        cache->mAllocTail = segment;
        segment->mPrev = NULL;
        segment->mNext = NULL;
        goto EXIT;
    }

    segment->mPrev = cache->mAllocTail;
    cache->mAllocTail->mNext = segment;
    cache->mAllocTail = segment;
    segment->mNext = NULL;

EXIT:

    SET_SEGMENT_BITMAP(cache, segment->mIndex);

    return;
}

static void delete_alloc_list(SegmentCache cache, Segment_t* segment)
{
    if(segment == NULL)
        return;

    if(segment->mPrev != NULL)
        segment->mPrev->mNext = segment->mNext;

    if(segment->mNext != NULL)
        segment->mNext->mPrev = segment->mPrev;

    if(cache->mAllocHead == segment)
        cache->mAllocHead = segment->mNext;

    if(cache->mAllocTail == segment)
        cache->mAllocTail = segment->mPrev;

    CLEAR_SEGMENT_BITMAP(cache, segment->mIndex);
}

static Segment_t* search_segment(SegmentCache cache, uint32_t segIndex)
{
    Segment_t* seg = cache->mAllocHead;

    if(!IS_SEGMENT_EXIST(cache, segIndex))
        return NULL;

    while(seg)
    {
        if(seg->mIndex == segIndex)
            break;

        seg = seg->mNext;
    }

    return seg;
}

static void cache_hit(SegmentCache cache, Segment_t* segment)
{
    if(segment == NULL)
        return;

    if(segment == cache->mAllocTail)
        return;

    delete_alloc_list(cache, segment);
    insert_alloc_list(cache, segment);
}

SegmentCache SegmentCache_Create(int64_t totalSize, uint32_t segmentSize, uint32_t maxSegmentToCache)
{
    SegmentCache cache = NULL;

    uint32_t  numOfSegment   = CEIL(totalSize, segmentSize);
    uint32_t  numOfSegBitmap = CEIL(numOfSegment, MAX_BITMAP_VALUE);

    cache = (SegmentCache)malloc(sizeof(SegmentCache_t) +
                                 sizeof(Bitmap_t) * numOfSegBitmap);

    if(cache == NULL)
        goto EXIT;

    cache->mTotalSize      = totalSize;
    cache->mSegmentSize    = segmentSize;
    cache->mNumOfSegment   = numOfSegment;

    cache->mNumOfSegBitmap = numOfSegBitmap;
    cache->mSegBitmap      = (Bitmap_t*)(cache + 1);
    memset(cache->mSegBitmap, 0x00, sizeof(Bitmap_t) * numOfSegBitmap);

    cache->mMaxSegmentToCache = maxSegmentToCache;

    create_segment_pool(cache, segmentSize, maxSegmentToCache);

EXIT:
    return cache;
}

void SegmentCache_Delete(SegmentCache cache)
{
    if(cache == NULL)
        return;

    delete_segment_pool(cache);
    free(cache);
}

BOOL SegmentCache_IsCached(SegmentCache cache, const SegmentInfo_t* info)
{
    if(info->mIndex >= cache->mNumOfSegment)
        return FALSE;

    return IS_SEGMENT_EXIST(cache, info->mIndex);
}

int SegmentCache_ReadCache(SegmentCache cache, int64_t offset, uint8_t* data, int len)
{
    uint32_t  relOffset;
    uint32_t segIdx = GET_BASE_SEGMENT_INDEX(cache, offset);
    Segment_t* seg = search_segment(cache, segIdx);

    if(seg == NULL)
    {
        fprintf(stderr, "Cannot found segment ! index : %d\n", segIdx);
        return -1;
    }

    relOffset = (uint32_t)(offset - GET_BASE_SEGMENT_OFFSET(cache, segIdx));

    len = MIN(len, seg->mSize - relOffset);

    memcpy(data, seg->mData + relOffset, len);

    cache_hit(cache, seg);

    return len;
}

int SegmentCache_WriteCache(SegmentCache cache, const SegmentInfo_t* info, const uint8_t* data)
{
    uint32_t copySize =0;
    Segment_t* seg = search_segment(cache, info->mIndex);

    if(seg == NULL)
    {
        if(IS_CACHE_FULL(cache))
        {
            seg = cache->mAllocHead;
            delete_alloc_list(cache, seg);
        }
        else
        {
            seg = alloc_segment(cache);
        }

        seg->mIndex   = info->mIndex;
        seg->mSize    = info->mSize;
        seg->mNext    = NULL;
        seg->mPrev    = NULL;

        insert_alloc_list(cache, seg);
    }

    memcpy(seg->mData, data, seg->mSize);
    cache_hit(cache, seg);

    return seg->mSize;
}

int SegmentCache_GetSegmentInfoByOffset(SegmentCache cache, int64_t offset, SegmentInfo_t* segInfo)
{
    Segment_t* seg = NULL;

    if(offset < 0 || offset >= cache->mTotalSize)
    {
        //fprintf(stderr, "invalid offset : %" PRIi64 "\n", offset);
        return -1;
    }

    segInfo->mIndex  = GET_BASE_SEGMENT_INDEX(cache, offset);
    segInfo->mOffset = GET_BASE_SEGMENT_OFFSET(cache, segInfo->mIndex);

    seg = search_segment(cache, segInfo->mIndex);
    if(seg == NULL)
        segInfo->mSize    = GET_SEGMENT_SIZE(cache, segInfo->mIndex);
    else
        segInfo->mSize    = seg->mSize;

    return 0;
}

void SegmentCache_Dump(SegmentCache cache)
{
    uint32_t ii;
    Segment_t* segment = cache->mAllocHead;

    printf("=====================================\n");
    printf("Total Size : %" PRIi64 "\n", cache->mTotalSize);
    printf("Segment Size  : %" PRIu32 "\n", cache->mSegmentSize);
    printf("Segment Count : %" PRIu32 "\n", cache->mNumOfSegment);

    printf("\n");
    printf("Dump Bitmap : \n");
    for(ii = 0; ii < cache->mNumOfSegBitmap; ii++)
        printf("  bitmap %d : %04x \n", ii, cache->mSegBitmap[ii]);

    printf("\n");
    printf("Dump Cached Segments : \n");

    while(segment)
    {
        printf("  segment %" PRIu32 " size : %" PRIu32 "\n", segment->mIndex, segment->mSize);
        segment = segment->mNext;
    }
    printf("=====================================\n");
}

