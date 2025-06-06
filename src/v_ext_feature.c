#include <unistd.h>
#include <time.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "v_common.h"
#include "v_misc.h"
#include "v_ext_feature.h"

#define EVENTFD_PATH "/tmp/ipc_fifo"

static void init_ops(void);
static void ext_init(void);
static void set_navigate_page_handler(v_navigate_page_callback_t);
static v_ext_info_t *get_status(v_ext_feature_t);
static void handle_commands(void);
static void ext_command_func_init(void);
static v_status_t v_init_external_receiver(void);
static v_status_t v_check_external_command(v_ext_command_t *ext_command);
static void on_status_notify(const v_ext_command_t *command);

static v_ext_ops_t _ops;
static v_ext_ops_t *ops;
static v_ext_info_t info[V_EXT_FEATURE_MAX];
static v_navigate_page_callback_t g_page_callback;

static void (*ext_command_func[V_EXT_COMMAND_MAX])(const v_ext_command_t *);

static int fd_in;
v_ext_ops_t *v_get_ext_ops(void)
{
    if (!ops)
        init_ops();
    return ops;
}

static void init_ops(void)
{
    _ops.init = ext_init;
    _ops.set_navigate_page_handler = set_navigate_page_handler;
    _ops.get_external_status = get_status;
    _ops.handle_external_commands = handle_commands;
    ops = &_ops;
}

static void ext_init(void)
{
    v_status_t status;
    status = v_init_external_receiver();
    if (status == ST_SUCCESS)
    {
        ext_command_func_init();
    }
}

static void set_navigate_page_handler(v_navigate_page_callback_t callback)
{
    g_page_callback = callback;
}

static v_hb_status_t get_hb_status(uint32_t last_updated)
{
    uint32_t now = v_current_time();
    int32_t d = now - last_updated;
    return ((0 <= d) && (d <= HEARTBEAT_THRESHOLD_SEC)) ? V_EXT_HB_ALIVE : V_EXT_HB_DEAD;
}

static v_ext_info_t *get_status(v_ext_feature_t feature)
{
    if (V_EXT_FEATURE_NONE < feature && feature < V_EXT_FEATURE_MAX)
    {
        info[feature].hb_status = get_hb_status(info[feature].last_updated);

        return &info[feature];
    }
    return NULL;
}

static void handle_commands(void)
{
    v_status_t status;
    v_ext_command_t ext_command;
    status = v_check_external_command(&ext_command);
    if (status == ST_SUCCESS)
    {
        // d("%d", ext_command.command);
        ext_command_func[ext_command.command](&ext_command);
    }
}

static v_status_t v_check_external_command(v_ext_command_t *ext_command)
{
    uint8_t *buf;
    v_status_t status = ST_EXT_RECEIVER_INIT_FAILED;
    static uint8_t prev_value = 0;
    ext_command->command = V_EXT_NONE;
    ERR_RET(fd_in <= 0, "efd open failed");

    ssize_t n = read(fd_in, &ext_command->option, sizeof(ext_command->option));

    if (n > 0)
    {
        status = ST_SUCCESS;
        buf = (uint8_t *)ext_command->option;

        switch (buf[0])
        {
        case V_IPC_COMMAND:
        {
            ERR_RETn(prev_value == buf[1]);

            d("%02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3]);
            prev_value = buf[1];
            if (buf[1] == 1)
            {
                ext_command->command = V_EXT_PAGE_NEXT;
            }
        }
        break;
        case V_IPC_STATUS_NOTIFY:
        {
            // through
            ext_command->command = V_EXT_STATUS_NOTIFY;
        }
        break;
        default:
            status = ST_EXT_NO_RECEIVE;
            break;
        }
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        status = ST_EXT_NO_RECEIVE;
    }
error_return:
    return status;
}

static void ext_on_page_changed(const v_ext_command_t *command)
{
    if (command->command == V_EXT_PAGE_PREV)
    {
        g_page_callback(V_EXT_PAGE_PREV);
    }
    else if (command->command == V_EXT_PAGE_NEXT)
    {
        g_page_callback(V_EXT_PAGE_NEXT);
    }
}

static void ext_command_func_init(void)
{
    ext_command_func[V_EXT_NONE] = ext_on_page_changed;
    ext_command_func[V_EXT_PAGE_PREV] = ext_on_page_changed;
    ext_command_func[V_EXT_PAGE_NEXT] = ext_on_page_changed;
    ext_command_func[V_EXT_STATUS_NOTIFY] = on_status_notify;
}

static v_status_t v_init_external_receiver(void)
{
    fd_in = 0;
    int ret;
    ret = mkfifo(EVENTFD_PATH, 0666);
    ERR_RET(ret < 0 && errno != EEXIST, "fd create fail");

    fd_in = open(EVENTFD_PATH, O_RDONLY | O_NONBLOCK);

error_return:
    return fd_in >= 0 ? ST_SUCCESS : ST_EXT_RECEIVER_INIT_FAILED;
}

static void on_status_notify(const v_ext_command_t *command)
{
    v_notify_data_t *notify = (v_notify_data_t *)command->option;

    dump((uint8_t *)command, 4);
    if (notify->feature <= V_EXT_FEATURE_NONE || notify->feature >= V_EXT_FEATURE_MAX)
        return;

    switch (notify->feature)
    {
    case V_EXT_FEATURE_BLE_RECEIVER:
    {
        if (info[notify->feature].hb_status == V_EXT_HB_DEAD || notify->bt.connected != info[notify->feature].bt.connected)
        {
            info[notify->feature].bt.connected = notify->bt.connected;
            info[notify->feature].bt.running = true;
        }
    }
    break;
    case V_EXT_FEATURE_MIDI:
    {
    }
    break;
    }
    info[notify->feature].last_updated = v_current_time();
}