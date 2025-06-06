#ifndef __V_EXT_FEATURE_H__
#define __V_EXT_FEATURE_H__

#include <stdint.h>

typedef uint8_t v_ipc_message_type_t;

#define V_IPC_STATUS_NOTIFY 0x10
#define V_IPC_COMMAND 0x30

#define HEARTBEAT_THRESHOLD_SEC 60

typedef uint8_t v_hb_status_t;
enum
{
    V_EXT_HB_DEAD,
    V_EXT_HB_ALIVE,
};

typedef uint8_t v_ext_command_type_t;
enum
{
    V_EXT_NONE,
    V_EXT_PAGE_NEXT,
    V_EXT_PAGE_PREV,
    V_EXT_STATUS_NOTIFY,
    V_EXT_COMMAND_MAX,
};

#pragma pack(push, 1)
typedef struct
{
    uint8_t command;
    uint8_t option[511];
} v_ext_command_t;

typedef struct str_v_ext_ble_status
{
    uint8_t running;
    uint8_t connected;
} v_ext_ble_status_t;

typedef struct str_v_ext_midi_status
{
    uint16_t bpm;
} v_ext_midi_status_t;
#pragma pack(pop)

typedef uint8_t v_ext_feature_t;
enum
{
    V_EXT_FEATURE_NONE,
    V_EXT_FEATURE_BLE_RECEIVER,
    V_EXT_FEATURE_MIDI,
    V_EXT_FEATURE_MAX
};

typedef struct str_v_ext_info
{
    v_ext_feature_t feature;
    v_hb_status_t hb_status;
    uint32_t last_updated;
    union
    {
        v_ext_ble_status_t bt;
        v_ext_midi_status_t midi;
    };
} v_ext_info_t;

#pragma pack(push, 1)
typedef struct str_v_notify_data
{
    v_ipc_message_type_t kind;
    v_ext_feature_t feature;
    union
    {
        v_ext_ble_status_t bt;
        v_ext_midi_status_t midi;
    };
} v_notify_data_t;
#pragma pack(pop)

typedef enum
{
    V_EXT_BLE_STATUS_UNKNOWN,
    V_EXT_BLE_STATUS_CONNECT,
    V_EXT_BLE_STATUS_DISCONNECT,
    V_EXT_BLE_STATUS_MAX,
} v_ext_ble_connection_status_t;

typedef void (*v_navigate_page_callback_t)(v_ext_command_type_t direction);

typedef struct str_v_ext_ops
{
    void (*init)(void);
    void (*set_navigate_page_handler)(v_navigate_page_callback_t callback);
    v_ext_info_t *(*get_external_status)(v_ext_feature_t feature);
    void (*handle_external_commands)(void);
} v_ext_ops_t;

v_ext_ops_t *v_get_ext_ops(void);
#endif
