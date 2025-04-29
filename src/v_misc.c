#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

#include "v_common.h"
#include "v_misc.h"
#include <math.h>

typedef uint8_t misc_filetype_t;
enum
{
    MISC_FILETYPE_REG,
    MISC_FILETYPE_DIR,
    MISC_FILETYPE_MAX,
};

static bool inode_exists(const char *path, misc_filetype_t t);
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

static char debug_str[MAX_PATH];
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

bool directory_exists(const char *path)
{
    return inode_exists(path, MISC_FILETYPE_DIR);
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
    return inode_exists(path, MISC_FILETYPE_REG);
}

char *execute_command(const char *command, ...)
{
    static char result[MAX_PATH] = {0};
    char cmd[MAX_PATH];
    va_list ap;
    sprintf(cmd, "%s", command);

    char *arg;
    va_start(ap, command);

    while ((arg = va_arg(ap, char *)) != NULL)
    {
        sprintf(&cmd[strlen(cmd)], " %s", arg);
    }

    va_end(ap);

    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        perror("popen failed");
        return NULL;
    }

    while (fgets(result, sizeof(result), pipe))
        ;

    int status = pclose(pipe);
    if (status == -1)
    {
        perror("pclose failed");
        return NULL;
    }
    else if (WEXITSTATUS(status) != 0)
    {
        fprintf(stderr, "Command failed with exit code %d\n", WEXITSTATUS(status));
        return NULL;
    }

    return result;
}

size_t get_file_size(const char *file)
{
    size_t size = 0;
    struct stat st;
    ERR_RETn(stat(file, &st));

    if (S_ISREG(st.st_mode))
    {
        size = st.st_size;
    }
error_return:
    return size;
}

const char *next_file_name(const char *orig)
{
    const char *name = NULL;
    ERR_RETn(!orig);
    int len = strlen(orig);
    int nlen;
    ERR_RETn(!len);
    const char *p = NULL;
    char *_name;

    for (int i = len - 1; i >= 0; i--)
    {
        if (orig[i] == '.')
        {
            if (i > 2)
            {
                p = &orig[i];
            }
            break;
        }
    }
    ERR_RETn(!p);

    bool has_suffix = false;
    if (p[-2] == '_')
    {
        if ('0' <= p[-1] && p[-1] <= '8')
        {
            has_suffix = true;
        }
    }

    nlen = has_suffix ? len : len + 2;

    _name = malloc(nlen + 1);
    ERR_RETn(!_name);
    strcpy(_name, orig);
    int offset = p - orig;
    if (!has_suffix)
    {
        _name[offset] = '_';
        offset += 2;
    }
    memcpy(&_name[offset], p, len - (p - orig));
    _name[offset - 1] = has_suffix ? (p[-1] + 1) : '1';

    name = _name;
error_return:
    return name;
}

static bool inode_exists(const char *path, misc_filetype_t t)
{
    struct stat st;
    bool ret = false;

    ERR_RETn(stat(path, &st));
    switch (t)
    {
    case MISC_FILETYPE_DIR:
        ret = S_ISDIR(st.st_mode);
        break;
    case MISC_FILETYPE_REG:
        ret = S_ISREG(st.st_mode);
        break;
    default:
        break;
    }

error_return:
    return ret;
}

char *get_dir_name(const char *path)
{
    ERR_RETn(!path);
    int len = strlen(path);
    ERR_RETn(!len);

    char *ret = NULL;
    char *d = strdup(path);

    ERR_RETn(!d);

    char *p;
    for (p = &d[len - 1]; p > d; p--)
    {
        if (*p == '/')
            break;
    }

    if (p > d)
    {
        memcpy(d, path, p - d);
        *p = '\0';
    }

    ret = d;
error_return:
    return ret;
}

char *b2s(uint8_t *data, uint32_t len)
{
    uint8_t upper;
    uint8_t lower;
    char *p = debug_str;

    for (uint32_t i = 0; i < len; i++)
    {
        upper = (data[i] & 0xf0) >> 4;
        lower = (data[i] & 0x0f) >> 0;

        *p++ = b2c(upper);
        *p++ = b2c(lower);
        *p++ = ' ';
    }
    *p++ = '\0';
    return debug_str;
}

int64_t npow(int64_t a, int64_t n)
{
    int64_t result = 1;
    while (n > 0)
    {
        if (n & 1)
        {
            result *= a;
        }
        a *= a;
        n >>= 1;
    }
    return result;
}