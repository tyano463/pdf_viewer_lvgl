#include <pthread.h>
#include <sys/queue.h>
#include <unistd.h>
#include "v_canvas.h"
#include "v_pen.h"

lv_obj_t *canvas;
LV_DRAW_BUF_DEFINE_STATIC(canvas_buf, WIDTH, HEIGHT, LV_COLOR_FORMAT_ARGB8888);
static lv_layer_t layer;
static pthread_cond_t cond;
static pthread_t *th;
static pthread_mutex_t *mutex;
static bool running;

static lv_img_dsc_t *dsc;
static lv_obj_t *image;

static void *draw_main(void *);

static void draw_thread_init(void)
{

    mutex = lv_malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    th = lv_malloc(sizeof(pthread_t));
    running = true;
    pthread_create(th, NULL, draw_main, NULL);
}

v_status_t v_init_canvas(lv_obj_t *parent)
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

v_status_t v_show_image(v_image_t *im)
{
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

void v_set_touch_callback(lv_event_cb_t cb)
{
    lv_obj_add_event_cb(canvas, cb, LV_EVENT_PRESSED | LV_EVENT_RELEASED, NULL);
}