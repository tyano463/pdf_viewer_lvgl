#include <pthread.h>
#include <sys/queue.h>
#include <unistd.h>
#include <stdlib.h>
#include "v_canvas.h"
#include "v_pen.h"

static void *draw_main(void *);
static void init_ops(void);
static void v_add_annot(v_annot_t *annot);
static void v_remove_annot(v_annot_t *annot);
static void v_show_frame(v_annot_t *annot);
static void v_set_show_mode(v_show_mode_t mode);
static v_status_t v_init_canvas(lv_obj_t *parent);
static v_status_t v_show_image(v_image_t *im, float);
static void v_set_touch_callback(lv_event_cb_t cb);
static void v_show_annot(void);
static void v_hide_annot(void);
static void v_queue(v_draw_event_t *ev);
extern lv_font_t source_hans_16;
extern lv_font_t source_hans_20;
extern lv_font_t source_hans_24;

lv_obj_t *canvas;

LV_DRAW_BUF_DEFINE_STATIC(canvas_buf, WIDTH, HEIGHT, LV_COLOR_FORMAT_ARGB8888);
static pthread_t *th;
static pthread_mutex_t *mutex;
static bool running;
static v_show_mode_t g_mode;

static lv_img_dsc_t *dsc;
static lv_obj_t *image;

static v_viewer_ops_t ops;
static float g_scale;

TAILQ_HEAD(tq_head, str_v_draw_event)
_head;
static struct tq_head *head;
TAILQ_HEAD(an_head, str_v_annot)
_ahead;
static struct an_head *an_head;
static void (*draw_func[V_DRAW_KIND_MAX])(v_draw_event_t *event);

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
        ops.show_annot = v_show_annot;
        ops.hide_annot = v_hide_annot;
        ops.queue = v_queue;
    }
}

static lv_font_t *get_font(uint8_t font_size)
{
    if (font_size < 16)
    {
        return &source_hans_16;
    }
    else if (font_size < 20)
    {
        return &source_hans_20;
    }
    else
    {
        return &source_hans_24;
    }
}

static void v_add_freetext(v_annot_t *annot)
{
    v_freetext_t *t = &annot->data.freetext;

    int16_t w, h;
    w = lv_obj_get_width(canvas);
    h = lv_obj_get_width(canvas);

    annot->pdf_annot_obj = (void *)lv_canvas_create(image);
    lv_draw_buf_t *d = malloc(sizeof(lv_draw_buf_t) + w * h * 4);
    lv_obj_t *a = annot->pdf_annot_obj;
    lv_draw_buf_init(d, w, h, LV_COLOR_FORMAT_ARGB8888, w * 4, &d[1], w * h * 4);
    lv_draw_buf_set_flag(d, LV_IMAGE_FLAGS_MODIFIABLE);
    lv_canvas_set_draw_buf(a, d);
    lv_canvas_fill_bg(a, lv_color_hex3(0xccc), LV_OPA_TRANSP);
    lv_layer_t l;

    lv_image_dsc_t *imdsc = (lv_image_dsc_t *)lv_image_get_src(image);
    int32_t ox = (w - imdsc->header.w) / 2;
    int32_t oy = 0;
    lv_canvas_init_layer(a, &l);

    lv_draw_label_dsc_t *dsc = calloc(sizeof(lv_draw_label_dsc_t), 1);
    lv_draw_label_dsc_init(dsc);
    dsc->color.red = t->color.c.red;
    dsc->color.green = t->color.c.green;
    dsc->color.blue = t->color.c.blue;
    dsc->text = t->content;
    dsc->font = get_font(t->font_size);

    lv_area_t coord;
    coord.x1 = (int32_t)t->position.left * g_scale + ox;
    coord.y1 = (int32_t)t->position.top * g_scale + oy;
    coord.x2 = (int32_t)t->position.right * g_scale + ox;
    coord.y2 = (int32_t)t->position.bottom * g_scale + oy;

    lv_draw_label(&l, dsc, &coord);
    d("%s %d,%d,%d,%d", dsc->text, coord.x1, coord.y1, coord.x2, coord.y2);
    lv_canvas_finish_layer(a, &l);
}

static void v_add_inklist(v_annot_t *annot)
{
    v_inklist_t *il = &annot->data.inklist;

    lv_image_dsc_t *imdsc = (lv_image_dsc_t *)lv_image_get_src(image);
    int16_t w, h;
    w = lv_obj_get_width(canvas);
    h = lv_obj_get_height(canvas);
    int32_t ox = (w - imdsc->header.w) / 2;
    int32_t oy = 0;

    annot->pdf_annot_obj = (void *)lv_canvas_create(image);
    lv_draw_buf_t *d = malloc(sizeof(lv_draw_buf_t) + w * h * 4);
    lv_obj_t *a = annot->pdf_annot_obj;
    lv_draw_buf_init(d, w, h, LV_COLOR_FORMAT_ARGB8888, w * 4, &d[1], w * h * 4);
    lv_draw_buf_set_flag(d, LV_IMAGE_FLAGS_MODIFIABLE);
    lv_canvas_set_draw_buf(a, d);
    lv_canvas_fill_bg(a, lv_color_hex3(0xccc), LV_OPA_TRANSP);
    lv_layer_t l;

    lv_obj_set_size(a, w, h);

    lv_canvas_init_layer(a, &l);
    for (int i = 0; i < il->num; i++)
    {
        v_stroke_t *st = &il->strokes[i];
        lv_draw_line_dsc_t *dsc = lv_malloc(sizeof(lv_draw_line_dsc_t));
        lv_draw_line_dsc_init(dsc);
        for (int j = 1; j < st->num; j++)
        {
            dsc->p1.x = st->points[j - 1].x * g_scale + ox;
            dsc->p1.y = st->points[j - 1].y * g_scale + oy;
            dsc->p2.x = st->points[j].x * g_scale + ox;
            dsc->p2.y = st->points[j].y * g_scale + oy;
            dsc->color.red = il->pen.color.c.red;
            dsc->color.green = il->pen.color.c.green;
            dsc->color.blue = il->pen.color.c.blue;
            dsc->opa = il->pen.color.c.alpha;
            dsc->width = il->pen.size;
            lv_draw_line(&l, dsc);
        }
    }
    lv_canvas_finish_layer(a, &l);
}

static void v_add_annot(v_annot_t *annot)
{
    ERR_RETn(!annot);

    void (*func[])(v_annot_t *) = {
        (void (*)(v_annot_t *))v_add_inklist,
        (void (*)(v_annot_t *))v_add_freetext,
    };

    TAILQ_INSERT_TAIL(an_head, annot, entry);
    func[annot->kind](annot);

error_return:
    return;
}
static void v_set_annot_visibility(bool vis)
{
    void (*func)(lv_obj_t *, lv_obj_flag_t) = vis ? lv_obj_remove_flag : lv_obj_add_flag;
    func(canvas, LV_OBJ_FLAG_HIDDEN);
}
static void v_hide_annot(void)
{
    v_set_annot_visibility(false);
}
static void v_show_annot(void)
{
    v_set_annot_visibility(true);
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

static void v_queue(v_draw_event_t *ev)
{
    pthread_mutex_lock(mutex);
    TAILQ_INSERT_TAIL(head, ev, entry);
    pthread_mutex_unlock(mutex);
}

static void draw_annot(v_draw_event_t *e) {}
static void draw_erase(v_draw_event_t *e) {}
static void show_all(v_draw_event_t *e)
{
    v_set_annot_visibility(false);
}
static void hide_all(v_draw_event_t *e)
{

    v_set_annot_visibility(true);
}
static void draw_user(v_draw_event_t *e)
{
    ERR_RETn(!e || !e->arg);
    ERR_RETn(!e->user_callback);
    e->user_callback(e->arg);
error_return:
    return;
}
static void draw_func_init(void)
{
    draw_func[V_DRAW_KIND_ANNOT] = draw_annot;
    draw_func[V_DRAW_KIND_ERASE] = draw_erase;
    draw_func[V_DRAW_KIND_SHOW_ALL] = show_all;
    draw_func[V_DRAW_KIND_HIDE_ALL] = hide_all;
    draw_func[V_DRAW_KIND_USER] = draw_user;
}
static void draw_thread_init(void)
{
    head = &_head;
    an_head = &_ahead;
    TAILQ_INIT(head);
    TAILQ_INIT(an_head);
    mutex = lv_malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    th = lv_malloc(sizeof(pthread_t));
    draw_func_init();
    running = true;
    pthread_create(th, NULL, draw_main, NULL);
}

static void font_load(void)
{
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

    font_load();
    draw_thread_init();

    status = ST_SUCCESS;
error_return:
    return status;
}

static void reset_annotation(void)
{
    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_TRANSP);
}

static v_status_t v_show_image(v_image_t *im, float current_scale)
{
    //    int w, h;
    //    float scale, scale_x, scale_y;
    if (dsc->data)
    {
        lv_free((uint8_t *)dsc->data);
    }
    reset_annotation();
    d("scale:%.02f", current_scale);

    g_scale = current_scale;
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
    v_draw_event_t *event;
    while (running)
    {
        pthread_mutex_lock(mutex);
        if (TAILQ_FIRST(head))
        {
            event = TAILQ_FIRST(head);
            TAILQ_REMOVE(head, event, entry);
            pthread_mutex_unlock(mutex);
            if (event->kind < V_DRAW_KIND_MAX)
            {
                draw_func[event->kind](event);
            }
            else
            {
                d("");
            }
        }
        else
        {
            pthread_mutex_unlock(mutex);
        }
        usleep(10000);
    }
    return NULL;
}

static void v_set_touch_callback(lv_event_cb_t cb)
{
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(canvas, cb, LV_EVENT_ALL, NULL);
}