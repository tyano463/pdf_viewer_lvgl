#include "lvgl/lvgl.h"
#include "v_icon.h"
#include "v_misc.h"
#include "v_common.h"

#pragma pack(push, 1)
typedef struct
{
    uint16_t reserved;
    uint16_t type;
    uint16_t count;
} IconDir;

typedef struct
{
    uint8_t width;
    uint8_t height;
    uint8_t colorCount;
    uint8_t reserved;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t imageSize;
    uint32_t offset;
} IconDirEntry;

#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint16_t bfType; // 'BM'
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits; // 画素データのオフセット
} BITMAPFILEHEADER;

typedef struct
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount; // 32ならARGB（実際はBGRA）
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

uint8_t *load_icon_data(uint8_t *ico, uint8_t *w, uint8_t *h)
{
    uint8_t *data = NULL;
    size_t pixel_size;
    IconDir *icon = (IconDir *)ico;
    int i;

    d("ico:%p", ico);

    d("icon->type:%d", icon->type);
    ERR_RETn(icon->type != 1);

    IconDirEntry *entries = (IconDirEntry *)&ico[sizeof(IconDir)];

    ERR_RETn(entries->bitCount != 32);

    pixel_size = entries->width * entries->height * 4;
    data = lv_malloc(pixel_size);
    d("%d,%d *4= %d off:%x im:%d", entries->width, entries->height, pixel_size, entries->offset, entries->imageSize);
    *w = entries->width;
    *h = entries->height;
    uint8_t *o = &ico[entries->offset];
    d("from o:%p-%p p:%p-%p", o, o + (*w) * (*h) * 4, data, &data[(*w) * (*h) * 4]);
    //    dump(o, 4096);
    for (int i = (*h) - 1; i >= 0; i--)
    {
        for (int j = 0; j < (*w); j++)
        {
            uint8_t *p = &data[i * (*w) * 4 + j * 4];
            p[0] = o[0];
            p[1] = o[1];
            p[2] = o[2];
            p[3] = o[3];
            o += 4;
        }
    }
    // lv_memcpy(data, ((uint8_t *)&entries[1]) + entries->offset, pixel_size);

    d("");
error_return:
    return data;
}

uint8_t *load_bmp_data(uint8_t *bmp, uint8_t *w, uint8_t *h)
{
    BITMAPFILEHEADER *bmpfile;
    BITMAPINFOHEADER *bmpinfo;
    uint8_t *data = NULL;

    bmpfile = (BITMAPFILEHEADER*)bmp;
    bmpinfo = (BITMAPINFOHEADER *)&bmpfile[1];

    *w = bmpinfo->biWidth;
    *h = bmpinfo->biHeight;

    data = lv_malloc((*w) * (*h) * 4);
    ERR_RETn(!data);
    uint8_t *o = &bmp[bmpfile->bfOffBits];
    for (int i = (*h) - 1; i >= 0; i--)
    {
        for (int j = 0; j < (*w); j++)
        {
            uint8_t *p = &data[i * (*w) * 4 + j * 4];
            p[0] = o[0];
            p[1] = o[1];
            p[2] = o[2];
            p[3] = o[3];
            o += 4;
        }
    }
error_return:
    return data;
}