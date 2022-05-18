#include "id_gen.h"

#include <stdint.h>
#include <stdio.h>

#define MAX_ID    (256)

typedef uint32_t  Bitmap_t;
#define MAX_BITMAP_VALUE (sizeof(Bitmap_t) * 8)

#define BITMAP_CNT  (MAX_ID / MAX_BITMAP_VALUE)
static  Bitmap_t    gIdBitmap[BITMAP_CNT];

int alloc_id()
{
    int ii, jj;
    for (int ii = 0; ii < BITMAP_CNT; ii++)
    {
        if (gIdBitmap[ii] == 0xFFFFFFFF)
            continue;

        Bitmap_t bitMask = 0x01;
        for (int jj = 0; jj < MAX_BITMAP_VALUE; jj++)
        {
            if (!(gIdBitmap[ii] & bitMask))
            {
                gIdBitmap[ii] |= bitMask;
                return (ii * MAX_BITMAP_VALUE) + jj;
            }

            bitMask <<= 1;
        }
    }

    printf("all id is allocated !\n");

    return -1;
}

void release_id(int id)
{
    if (id < 0 || id >= MAX_ID)
        return;

    int index = id / MAX_BITMAP_VALUE;
    int shift   = id % MAX_BITMAP_VALUE;

    Bitmap_t bitMask = 0x01 << shift;

    gIdBitmap[index] &= (~bitMask);
}
