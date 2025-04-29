#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "sjis.txt"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

uint16_t sjis_to_utf16be(uint16_t sjis)
{
    size_t table_size = ARRAY_SIZE(mapping_table);
    mapping_t *table = mapping_table;
    size_t left = 0;
    size_t right = table_size;

    while (left < right)
    {
        size_t mid = left + (right - left) / 2;

        if (table[mid].sjis == sjis)
        {
            return table[mid].unicode;
        }
        else if (table[mid].sjis < sjis)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return 0x20;
}

size_t utf16be_char_to_utf8(uint16_t code_point, char *utf8_buffer, size_t utf8_buffer_size)
{
    size_t utf8_index = 0;

    if (code_point <= 0x007F)
    {
        if (utf8_index + 1 <= utf8_buffer_size)
        {
            utf8_buffer[utf8_index++] = code_point & 0x7F;
        }
    }
    else if (code_point <= 0x07FF)
    {
        if (utf8_index + 2 <= utf8_buffer_size)
        {
            utf8_buffer[utf8_index++] = 0xC0 | ((code_point >> 6) & 0x1F);
            utf8_buffer[utf8_index++] = 0x80 | (code_point & 0x3F);
        }
    }
    else
    {
        if (utf8_index + 3 <= utf8_buffer_size)
        {
            utf8_buffer[utf8_index++] = 0xE0 | ((code_point >> 12) & 0x0F);
            utf8_buffer[utf8_index++] = 0x80 | ((code_point >> 6) & 0x3F);
            utf8_buffer[utf8_index++] = 0x80 | (code_point & 0x3F);
        }
    }

    utf8_buffer[utf8_index] = '\0';
    return utf8_index;
}

char *sjis_to_utf8(uint8_t *data, size_t len)
{
    size_t cur_len = len;
    char *utf8_base = malloc(cur_len);
    char *utf8 = utf8_base;

    int osize = 0;
    for (uint8_t *p = data; (p - data) < len; p += osize)
    {
        uint16_t orig;
        uint16_t converted;
        if ((*p) < 0x80 || (0xa1 <= (*p) && (*p) <= 0xdf))
        {
            osize = 1;
            orig = (uint16_t)(uint8_t)(p[0] & 0xff);
        }
        else
        {
            osize = 2;
            orig = p[1] & 0xff;
            orig += (uint16_t)((p[0] << 8) & 0xffff);
        }
        converted = sjis_to_utf16be(orig);
        uint32_t valid;
        if ((&utf8_base[cur_len] - utf8) < 5)
        {
            cur_len += 0x10;
            size_t offset = utf8 - utf8_base;
            utf8_base = realloc(utf8_base, cur_len);
            utf8 = &utf8_base[offset];
//            printf("utf8:%p base:%p offset:%lx\n", utf8, utf8_base, offset);
        }
        valid = utf16be_char_to_utf8(converted, utf8, 4);
        utf8[valid] = '\0';
        utf8 += valid;
    }
    return utf8_base;
}
