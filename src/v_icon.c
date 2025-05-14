#include "lvgl/lvgl.h"
#include <stdio.h>
#include "v_icon.h"
#include "v_misc.h"
#include "v_common.h"
#include "v_assets_list.h"

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
    const BITMAPFILEHEADER *bmpfile;
    const BITMAPINFOHEADER *bmpinfo;
    uint8_t *data = NULL;

    bmpfile = (BITMAPFILEHEADER *)bmp;
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

void to_bmp(const char *path, const uint8_t *data, uint16_t width, uint16_t height)
{
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    memset(&file_header, 0, sizeof(BITMAPFILEHEADER));
    memset(&info_header, 0, sizeof(BITMAPINFOHEADER));

    int row_size = width * 4;
    int image_size = row_size * height;
    int file_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_size;

    file_header.bfType = 0x4D42;
    file_header.bfSize = file_size;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    info_header.biSize = sizeof(BITMAPINFOHEADER);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 32;
    info_header.biCompression = 3;
    info_header.biSizeImage = image_size;
    info_header.biXPelsPerMeter = 0;
    info_header.biYPelsPerMeter = 0;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;

    // ファイルを開いて書き込む
    FILE *file = fopen(path, "wb");
    if (!file)
    {
        perror("Unable to open file for writing");
        return;
    }

    fwrite(&file_header, sizeof(file_header), 1, file);
    fwrite(&info_header, sizeof(info_header), 1, file);

    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < width; x++)
        {
            unsigned char b = data[(y * width + x) * 4 + 0];
            unsigned char g = data[(y * width + x) * 4 + 1];
            unsigned char r = data[(y * width + x) * 4 + 2];
            unsigned char a = data[(y * width + x) * 4 + 3];
            fwrite(&b, sizeof(unsigned char), 1, file);
            fwrite(&g, sizeof(unsigned char), 1, file);
            fwrite(&r, sizeof(unsigned char), 1, file);
            fwrite(&a, sizeof(unsigned char), 1, file);
        }
    }

    fclose(file);
}

lv_image_dsc_t *get_icon_dsc(const char *name)
{
    lv_image_dsc_t *dsc = lv_malloc(sizeof(lv_image_dsc_t));
    lv_memzero(dsc, sizeof(lv_image_dsc_t));

    uint8_t w, h;
    uint8_t *asset = get_asset_ptr(name);
    d("asset:%p", asset);
    dsc->data = load_bmp_data(asset, &w, &h);

    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->data_size = w * h * 4;
    dsc->header.stride = w * 4;
    dsc->header.flags = 0;
    dsc->header.w = w;
    dsc->header.h = h;
    dsc->header.cf = LV_COLOR_FORMAT_ARGB8888;

    return dsc;
}
