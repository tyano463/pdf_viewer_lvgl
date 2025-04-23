#include <string.h>
#include "v_assets_list.h"

#define ENTRY(name) extern uint8_t assets_##name##_bmp[];
ASSETS_LIST
#undef ENTRY

typedef struct {
    const char *name;
    uint8_t *ptr;
} AssetEntry;

#define ENTRY(name) { #name, assets_##name##_bmp },
static AssetEntry asset_table[] = {
    ASSETS_LIST
    { NULL, NULL }
};
#undef ENTRY

uint8_t *get_asset_ptr(const char *name) {
    for (int i = 0; asset_table[i].name != NULL; i++) {
        if (strcmp(asset_table[i].name, name) == 0) {
            return asset_table[i].ptr;
        }
    }
    return NULL;
}