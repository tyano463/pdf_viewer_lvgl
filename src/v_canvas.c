#include <pthread.h>
#include <sys/queue.h>
#include <unistd.h>
#include "v_canvas.h"
#include "v_pen.h"

static void *draw_main(void *);
static void init_ops(void);
static void v_add_annot(v_annot_t *annot);
static void v_remove_annot(v_annot_t *annot);
static void v_show_frame(v_annot_t *annot);
static void v_set_show_mode(v_show_mode_t mode);
static v_status_t v_init_canvas(lv_obj_t *parent);
static v_status_t v_show_image(v_image_t *im);
static void v_set_touch_callback(lv_event_cb_t cb);

lv_obj_t *canvas;
LV_DRAW_BUF_DEFINE_STATIC(canvas_buf, WIDTH, HEIGHT, LV_COLOR_FORMAT_ARGB8888);
static lv_layer_t layer;
static pthread_cond_t cond;
static pthread_t *th;
static pthread_mutex_t *mutex;
static bool running;
static v_show_mode_t g_mode;

static lv_img_dsc_t *dsc;
static lv_obj_t *image;

static v_viewer_ops_t ops;

static void init_ops(void)
{
    if (!ops.init)
    {
        ops.init = v_init_canvas;
        ops.show_image = v_show_image;
        ops.set_touch_callback = v_set_touch_callback;
        ops.set_mode = v_set_show_mode;
        ops.show_frame = v_show_frame;
        ops.add_annot = v_add_annot;
        ops.remove_annot = v_remove_annot;
    }
}

static void v_add_annot(v_annot_t *annot)
{
}
static void v_remove_annot(v_annot_t *annot)
{
}

static void v_show_frame(v_annot_t *annot)
{
}
static void v_set_show_mode(v_show_mode_t mode)
{
    g_mode = mode;
}

v_viewer_ops_t *v_get_canvas_ops(void)
{
    init_ops();
    return &ops;
}
static void draw_thread_init(void)
{

    mutex = lv_malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    th = lv_malloc(sizeof(pthread_t));
    running = true;
    pthread_create(th, NULL, draw_main, NULL);
}

static v_status_t v_init_canvas(lv_obj_t *parent)
{
    v_status_t status = ST_CREATE_CANVAS_FAILED;

    dsc = lv_malloc(sizeof(lv_img_dsc_t));
    ERR_RET(!dsc, "malloc");
    memset(dsc, 0, sizeof(lv_img_dsc_t));
    image = lv_img_create(parent);
    ERR_RET(!image, "lv_img_create");

    lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 0);

    canvas = lv_canvas_create(parent);
    ERR_RET(!canvas, "lv_canvas_create");

    LV_DRAW_BUF_INIT_STATIC(canvas_buf);

    lv_canvas_set_draw_buf(canvas, &canvas_buf);
    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_TRANSP);
    lv_obj_center(canvas);

    draw_thread_init();

    status = ST_SUCCESS;
error_return:
    return status;
}

static v_status_t v_show_image(v_image_t *im)
{
    int w, h;
    float scale, scale_x, scale_y;
    if (dsc->data)
    {
        lv_free((uint8_t *)dsc->data);
    }

    uint8_t *data = lv_malloc(im->size);
    lv_memcpy(data, im->buf, im->size);

    dsc->data = data;
    dsc->header.cf = im->format;
    dsc->header.w = im->width;
    dsc->header.h = im->height;
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->data_size = im->size;
    lv_img_set_src(image, dsc);
    return ST_SUCCESS;
}

static void *draw_main(void *arg)
{
    while (running)
    {
        pthread_mutex_lock(mutex);

        pthread_mutex_unlock(mutex);
        usleep(10000);
    }
    return NULL;
}

static void v_set_touch_callback(lv_event_cb_t cb)
{
    lv_obj_add_event_cb(canvas, cb, LV_EVENT_PRESSED | LV_EVENT_RELEASED, NULL);
}