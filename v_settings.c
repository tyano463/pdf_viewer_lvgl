#include "v_settings.h"
#include <stdio.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#define V_SETTINGS_FILE "settings.json"
#define V_CONFIG_DIR "/.config/" APP_NAME
static v_settings_t settings;

static const char *v_config_dir(void) {
    const char *config_home = getenv("XDG_CONFIG_HOME");
    char *path = NULL;
    if (!config_home) {
        const char *home = getenv("HOME");
        if (!home) {
            config_home = "/root" V_CONFIG_DIR;
            return config_home;
        } else {
            int len = strlen(home);
            path = malloc(len + 1 + strlen(V_CONFIG_DIR));
            sprintf(path,"%s", home, V_CONFIG_DIR);
            return path;
        }
    } else {
        int len = strlen(config_home);
        path = malloc(len + 1 + strlen("/" APP_NAME));
        sprintf(path, "%s/%s", config_home, APP_NAME);
        return path;
    }
}

static bool exists_directory(const char * s){
    return true;
}
v_status_t v_save_settings(void) {
    const char *config_dir = v_config_dir();
    if (!exists_directory(config_dir)) {

    }
}
v_status_t v_load_settings(void)
{
    return ST_SUCCESS;
}