#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "v_common.h"

static FILE *log_fp;
static char time_str[19];

static v_status_t log_rotate(void);
static bool exists(const char *);

v_status_t v_log_init(void)
{
    log_fp = fopen(V_LOG_DIR "/" V_LOG_FILE, "a");
    return (log_fp) ? ST_SUCCESS : ST_LOG_INIT_FAILED;
}

char *get_time_str(void)
{
    char *p = NULL;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *tm = localtime(&tv.tv_sec);
    ERR_RETn(!tm);

    snprintf(time_str, sizeof(time_str),
             "%02d%02d%02d_%02d%02d%02d.%03ld ",
             (tm->tm_year + 1900) % 100,
             tm->tm_mon + 1,
             tm->tm_mday,
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec,
             tv.tv_usec / 1000);

    p = time_str;
error_return:
    return p;
}

v_status_t v_log_write(const char *s, ...)
{
    v_status_t status = ST_LOG_WRITE_FAILED;

    ERR_RETn(!log_fp);

    fprintf(log_fp, "%s", get_time_str());
    va_list args;
    va_start(args, s);

    if (vfprintf(log_fp, s, args) < 0)
    {
        va_end(args);
        goto error_return;
    }

    fflush(log_fp);
    va_end(args);

    status = log_rotate();

error_return:
    return status;
}

static bool need_rotation(void)
{
    if (!log_fp)
    {
        return false;
    }
    return ftell(log_fp) > V_LOG_SIZE;
}

static v_status_t log_rotate(void)
{
    int i, len;
    v_status_t status = ST_SUCCESS;
    char s[2][MAX_PATH];
    char *old, *path;

    ERR_RETn(!need_rotation());

    status = ST_LOG_ROTATE_FAILED;

    fclose(log_fp);
    log_fp = NULL;

    path = s[V_LOG_MAX % 2];
    sprintf(path, "%s.%d", V_LOG_DIR "/" V_LOG_FILE, V_LOG_MAX);
    len = strlen(path);
    unlink(path);
    for (i = V_LOG_MAX; i > 0; i--)
    {
        old = s[(i - 1) % 2];
        sprintf(old, "%s.%d", V_LOG_DIR "/" V_LOG_FILE, V_LOG_MAX);
        if (exists(old))
            rename(old, path);
        path = old;
    }
    rename(V_LOG_DIR "/" V_LOG_FILE, path);
    status = v_log_init();

error_return:
    return status;
}

static bool exists(const char *path)
{
    return access(path, F_OK) == 0;
}