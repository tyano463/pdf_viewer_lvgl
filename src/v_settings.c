#include <stdio.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "v_common.h"
#include "v_settings.h"
#include "v_misc.h"

#define V_SETTINGS_FILE "settings.json"
#define V_CONFIG_DIR "/.config/" APP_NAME

// #define PDF_FILE "/usr/share/sample.pdf"
// #define PDF_FILE "/home/tyano/Documents/annot.pdf"
#define PDF_FILE "/home/tyano/Downloads/R02117G2.mid"
#define EN "en"
#define JA "ja"
#define FALLBACK_LANG EN

#define DEFAULT_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

static void init_ops(void);

static const char *s_valid_lang[] = {
    JA,
    EN,
};

static v_settings_t g_settings;
static v_settings_ops_t g_ops;

static char config_path[MAX_PATH];
static const char *v_config_dir(void)
{
    const char *config_home = getenv("XDG_CONFIG_HOME");
    char *path = NULL;
    if (!config_home)
    {
        const char *home = getenv("HOME");
        if (!home)
        {
            config_home = "/root" V_CONFIG_DIR;
            return config_home;
        }
        else
        {
            int len = strlen(home);
            path = malloc(len + 1 + strlen(V_CONFIG_DIR));
            sprintf(path, "%s", home, V_CONFIG_DIR);
            return path;
        }
    }
    else
    {
        int len = strlen(config_home);
        path = malloc(len + 1 + strlen("/" APP_NAME));
        sprintf(path, "%s/%s", config_home, APP_NAME);
        return path;
    }
}

static bool exists_directory(const char *s)
{
    struct stat st;

    if (stat(s, &st))
        return false;

    return S_ISDIR(st.st_mode);
}

static char *get_config_path(void)
{
    const char *config_dir = v_config_dir();
    if (!exists_directory(config_dir))
    {
        mkdir_p(config_dir, DEFAULT_MODE);
    }
    if (!exists_directory(config_dir))
        return NULL;
    sprintf(config_path, "%s/" V_SETTINGS_FILE, config_dir);
    return config_path;
}

v_status_t v_save_settings(void)
{
    v_status_t status;

    status = ST_SETTING_SAVE_FAILED;
    const char *path = get_config_path();

    FILE *fp = fopen(path, "w");

error_return:
    return status;
}

v_settings_ops_t *v_get_settings_ops(void)
{
    init_ops();
    return &g_ops;
}

static v_status_t serialize_settings_json(void)
{
    v_settings_t *p = &g_settings;
    v_status_t status;

    status = ST_SETTING_SERIALIZE_FAILED;
    cJSON *json = cJSON_CreateObject();
    ERR_RET(!json, "json");
    cJSON *last_path;
    cJSON *last_page;
    cJSON *annot;
    cJSON *pen;

    last_path = cJSON_CreateString(p->last_opened);
    ERR_RET(!last_path, V_S_ITEM_LAST_PATH);
    last_page = cJSON_CreateNumber(p->last_opened_page);
    ERR_RET(!last_page, V_S_ITEM_LAST_PAGE);
    annot = cJSON_CreateNumber(p->annot);
    ERR_RET(!annot, V_S_ITEM_ANNOT);
    pen = pen_to_json(&p->last_pen);
    ERR_RET(!pen, V_S_ITEM_LAST_PEN);

    cJSON_AddItemToObject(json, V_S_ITEM_LAST_PATH, last_path);
    cJSON_AddItemToObject(json, V_S_ITEM_LAST_PAGE, last_page);
    cJSON_AddItemToObject(json, V_S_ITEM_LAST_PEN, pen);
    cJSON_AddItemToObject(json, V_S_ITEM_ANNOT, annot);

    status = ST_SUCCESS;
error_return:
    if (json)
        cJSON_Delete(json);
    return status;
}

static v_status_t parse_settings_json(const char *const data)
{
    v_status_t status;

    status = ST_SETTING_PARSE_FAILED;

    cJSON *last_path;
    cJSON *last_page;
    cJSON *annot;
    cJSON *pen_json;
    cJSON *json = cJSON_Parse(data);
    ERR_RET(!json, "cJSON_Parse");

    v_settings_t *p = &g_settings;

    last_path = cJSON_GetObjectItemCaseSensitive(json, V_S_ITEM_LAST_PATH);
    ERR_RET(!last_page, V_S_ITEM_LAST_PATH);
    last_page = cJSON_GetObjectItemCaseSensitive(json, V_S_ITEM_LAST_PAGE);
    ERR_RET(!last_page, V_S_ITEM_LAST_PAGE);
    annot = cJSON_GetObjectItemCaseSensitive(json, V_S_ITEM_ANNOT);
    ERR_RET(!last_page, V_S_ITEM_ANNOT);
    pen_json = cJSON_GetObjectItemCaseSensitive(json, V_S_ITEM_LAST_PEN);
    ERR_RET(!last_page, V_S_ITEM_LAST_PEN);

    sprintf(p->last_opened, "%s", last_path->valuestring);
    p->last_opened_page = last_page->valueint;
    p->annot = annot->valueint;

    v_pen_t *pen = json_to_pen(pen_json);
    ERR_RET(!pen, "pen");

    memcpy(&p->last_pen, pen, sizeof(sizeof(p->last_pen)));
    status = ST_SUCCESS;

error_return:
    if (json)
        cJSON_free(json);
    if (last_page)
        cJSON_free(last_page);
    if (last_path)
        cJSON_free(last_path);
    if (pen_json)
        cJSON_free(pen_json);
    if (annot)
        cJSON_free(annot);
    return status;
}

v_status_t v_load_settings(void)
{
    v_status_t status;

    status = ST_SETTING_SAVE_FAILED;

    const char *path = get_config_path();
    ERR_RET(!path, "config path");

    FILE *fp = fopen(path, "r");
    ERR_RET(!fp, "fopen faild");

    ERR_RET(fseek(fp, 0, SEEK_END), "fseek");
    size_t size = ftell(fp);
    ERR_RET(!size, "ftell");

    rewind(fp);
    char *buf = malloc(size);
    ERR_RET(!buf, "malloc");

    ERR_RET(fread(buf, size, 1, fp), "fread");

    ERR_RET(parse_settings_json(buf) != ST_SUCCESS, "parse json");

error_return:
    if (fp)
        fclose(fp);
    return status;
}

static void set_pen(const v_pen_t *pen)
{
    memcpy(&g_settings.last_pen, pen, sizeof(v_pen_t));
}
static void set_page(uint16_t page)
{
    g_settings.last_opened_page = page;
}
static void set_path(const char *path)
{
    strcpy(g_settings.last_opened, path);
}
static void set_annot_mode(v_annot_display_t mode)
{
    g_settings.annot = mode;
}

static const char *get_path(void)
{
    d("last_opened: %s", g_settings.last_opened);
    if (!g_settings.last_opened[0])
    {
        return PDF_FILE;
    }
    return g_settings.last_opened;
}
static uint16_t get_page(void)
{
    return g_settings.last_opened_page;
}
static v_pen_t *get_pen(void)
{
    return &g_settings.last_pen;
}
static v_annot_display_t get_annot_mode(void)
{
    return g_settings.annot;
}

static void set_lang(const char *s)
{
    ERR_RETn(!s || !s[0]);

    snprintf(g_settings.lang, sizeof(g_settings.lang), "%.*s", sizeof(g_settings.lang) - 1, s);

error_return:
    return;
}

static bool valid_lang(void)
{
    int n = ARRAY_SIZE(s_valid_lang);
    for (int i = 0; i < n; i++)
    {
        if (strcmp(g_settings.lang, s_valid_lang[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

static const char *get_lang(void)
{
    if (valid_lang())
    {
        return g_settings.lang;
    }
    return FALLBACK_LANG;
}

static void init_ops(void)
{
    g_ops.set_page = set_page;
    g_ops.set_path = set_path;
    g_ops.set_annot_mode = set_annot_mode;
    g_ops.set_pen = set_pen;
    g_ops.set_lang = set_lang;

    g_ops.get_page = get_page;
    g_ops.get_path = get_path;
    g_ops.get_pen = get_pen;
    g_ops.get_annot_mode = get_annot_mode;
    g_ops.get_lang = get_lang;
}