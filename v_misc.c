#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "v_common.h"
#include "v_misc.h"
#include <math.h>

static char b2c(const uint8_t b)
{
    if (0 <= b && b <= 9)
    {
        return b + '0';
    }
    else if (0xa <= b && b <= 0xf)
    {
        return b + 'a' - 0xa;
    }
    else
    {
        return '.';
    }
}

static char debug_str[64];
void dump(const uint8_t *data, size_t size)
{
    char upper, lower;
    bool last_lf;
    char *p = debug_str;

    last_lf = false;
    for (int i = 0; i < size; i++)
    {
        if ((i % 16) == 0)
        {
            p += sprintf(p, "%04x: ", i);
        }
        upper = (char)((data[i] & 0xf0) >> 4);
        lower = (char)((data[i] & 0x0f) >> 0);
        p += sprintf(p, "%c%c", b2c(upper), b2c(lower));
        if ((i % 16) == 15)
        {
            p += sprintf(p, "\n");
            printf("%s", debug_str);
            p = debug_str;
            last_lf = true;
        }
        else
        {
            p += sprintf(p, " ");
            last_lf = false;
        }
    }
    if (!last_lf)
    {
        printf("%s\n", debug_str);
    }
}

float distance(lv_point_t *a, lv_point_t *b)
{
    float d2 = (b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y);
    return sqrtf(d2);
}

int mkdir_p(const char *path, mode_t mode)
{
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(tmp, mode) != 0)
            {
                if (errno != EEXIST)
                {
                    perror("mkdir");
                    return -1;
                }
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0)
    {
        if (errno != EEXIST)
        {
            perror("mkdir");
            return -1;
        }
    }

    return 0;
}

bool ends_with_ignore_case(const char *str, const char *suffix)
{
    size_t len_str = strlen(str);
    size_t len_suffix = strlen(suffix);
    if (len_str < len_suffix)
        return 0;

    const char *str_ext = str + len_str - len_suffix;
    while (*str_ext && *suffix)
    {
        if (tolower((unsigned char)*str_ext) != *suffix)
        {
            return false;
        }
        str_ext++;
        suffix++;
    }
    return true;
}

bool file_exists(const char *path)
{
    struct stat st;

    if (lstat(path, &st))
    {
        return false;
    }

    if (S_ISLNK(st.st_mode))
    {
        if (stat(path, &st))
            return false;
    }

    return S_ISREG(st.st_mode);
}