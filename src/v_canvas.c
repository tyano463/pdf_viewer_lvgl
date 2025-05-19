#include <pthread.h>
#include <sys/queue.h>
#include <unistd.h>
#include <stdlib.h>
#include <cairo/cairo.h>
#include "v_common.h"
#include "v_core.h"
#include "v_canvas.h"
#include "v_pen.h"
#include "v_icon.h"
#include "v_undo.h"

static void *draw_main(void *);
static void init_ops(void);
static void v_add_annot(v_annot_t *annot);
static void v_remove_annot(v_annot_t *annot);
static void v_set_show_mode(v_show_mode_t mode);
static v_status_t v_init_canvas(lv_obj_t *parent);
static v_status_t v_show_image(v_image_t *im);
static void v_set_touch_callback(lv_event_cb_t cb);
static void v_show_annot(void);
static void v_hide_annot(void);
static void v_queue(v_draw_event_t *ev);
static void v_select(lv_point_t *pos);
static void v_move(const lv_point_t *from, const lv_point_t *to);
static void on_remove_pressed(lv_event_t *);
static void on_resize_dragged(lv_event_t *);
static void show_annot_control(v_annot_t *a);
static void hide_annot_control(void);
static v_annots_t *v_all_annots(void);
static void set_scale_translate_matrix(v_matrix_t *m, float scale, float ox, float oy);
static void v_get_matrix(v_matrix_t *m);
static void v_undo(void);
static void v_redo(void);
static void init_rect(v_rect_t *rect);

extern lv_font_t source_hans_16;
extern lv_font_t source_hans_20;
extern lv_font_t source_hans_24;

lv_obj_t *canvas;

static lv_draw_buf_t *canvas_buf;
static pthread_t *th;
static pthread_mutex_t *mutex;
static bool running;
static v_show_mode_t g_mode;

static lv_img_dsc_t *g_dsc;
static lv_obj_t *image;

static v_annot_control_t _annot_control;
static v_annot_control_t *annot_control;

static v_viewer_ops_t ops;
static float g_scale;

TAILQ_HEAD(tq_head, str_v_draw_event)
_head;
static struct tq_head *head;
TAILQ_HEAD(an_head, str_v_annot)
_ahead;
static struct an_head *an_head;
static void (*draw_func[V_DRAW_KIND_MAX])(const v_draw_event_t *event);

static void init_ops(void)
{
    if (!ops.init)
    {
        ops.init = v_init_canvas;
        ops.show_image = v_show_image;
        ops.set_touch_callback = v_set_touch_callback;
        ops.set_mode = v_set_show_mode;
        ops.add_annot = v_add_annot;
        ops.remove_annot = v_remove_annot;
        ops.show_annot = v_show_annot;
        ops.hide_annot = v_hide_annot;
        ops.queue = v_queue;
        ops.select = v_select;
        ops.move = v_move;
        ops.annots = v_all_annots;
        ops.matrix = v_get_matrix;
        ops.undo = v_undo;
        ops.redo = v_redo;
    }
}

static v_annot_t *get_target_annot(uint32_t id)
{
    v_annot_t *ret = NULL;
    v_annot_t *iter;

    ERR_RET(!id, "invalid argument");

    TAILQ_FOREACH(iter, an_head, entry)
    {
        if (iter && iter->id == id)
        {
            ret = iter;
            break;
        }
    }
error_return:
    return ret;
}

static void clear_selection(void)
{
    hide_annot_control();
    annot_control->annot = NULL;
}

static void do_undo_redo(undo_entry_t *entry, v_undo_type_t kind)
{
    ERR_RETn(!entry);
    v_annot_t *target, *a, *b;

    b = entry->before;
    a = entry->after;

    d("action:%d %p -> %p", entry->action, b, a);

    clear_selection();

    switch (entry->action)
    {
    case UNDO_ACTION_NEW_INK:
        ERR_RET(!a, "undo failed");

        if (kind == V_UNDO_TYPE_REDO)
        {
            target = get_target_annot(a->id);
            ERR_RET(target, "target found");
            v_add_annot(a);
        }
        else
        {
            target = get_target_annot(a->id);
            ERR_RET(!target, "target not found");
            v_remove_annot(target);
        }
        break;
    case UNDO_ACTION_TRANSFORM_INK:
        ERR_RET(!a || !b, "undo failed");
        ERR_RET(a->id != b->id, "undo failed");
        target = get_target_annot(a->id);
        ERR_RET(!target, "target not found");

        v_remove_annot(target);
        if (kind == V_UNDO_TYPE_REDO)
        {
            memcpy(&target->matrix, &a->matrix, sizeof(v_matrix_t));
        }
        else
        {
            memcpy(&target->matrix, &b->matrix, sizeof(v_matrix_t));
        }
        v_add_annot(target);

        break;
    case UNDO_ACTION_DELETE_INK:
        ERR_RET(!b, "undo failed");
        target = get_target_annot(b->id);
        if (kind == V_UNDO_TYPE_REDO)
        {
            ERR_RET(!target, "target not found");
            v_remove_annot(target);
        }
        else
        {
            ERR_RET(target, "target found");
            v_add_annot(b);
        }
        break;
    case UNDO_ACTION_MOVE_TEXT:
        d("%p -> %p", entry->before, entry->after);
        break;
    case UNDO_ACTION_DELETE_TEXT:
        d("%p -> %p", entry->before, entry->after);
        break;
    default:
        d("invalid case");
        break;
    }

error_return:
    return;
}

static void v_undo(void)
{
    undo_entry_t *entry = undo_do();
    do_undo_redo(entry, V_UNDO_TYPE_UNDO);
}
static void v_redo(void)
{
    undo_entry_t *entry = redo_do();
    do_undo_redo(entry, V_UNDO_TYPE_REDO);
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
    v_freetext_t *t = annot->data.freetext;

    int16_t w, h;
    w = lv_obj_get_width(canvas);
    h = lv_obj_get_width(canvas);

    annot->pdf_annot_obj = (void *)lv_canvas_create(canvas);
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
    ERR_RET(!dsc, "calloc error");
    lv_draw_label_dsc_init(dsc);
    dsc->color.red = t->color.c.red;
    dsc->color.green = t->color.c.green;
    dsc->color.blue = t->color.c.blue;
    dsc->text = t->content;
    dsc->font = get_font(t->font_size);

    // TODO: matrixに変更
    lv_area_t coord;
    coord.x1 = (int32_t)t->position.left * g_scale + ox;
    coord.y1 = (int32_t)t->position.top * g_scale + oy;
    coord.x2 = (int32_t)t->position.right * g_scale + ox;
    coord.y2 = (int32_t)t->position.bottom * g_scale + oy;
    annot->rect.left = coord.x1;
    annot->rect.top = coord.y1;
    annot->rect.right = coord.x2;
    annot->rect.bottom = coord.y2;

    lv_draw_label(&l, dsc, &coord);
    d("%s %d,%d,%d,%d", dsc->text, coord.x1, coord.y1, coord.x2, coord.y2);
    lv_canvas_finish_layer(a, &l);
error_return:
    if (dsc)
        free(dsc);
    return;
}

static void v_get_matrix(v_matrix_t *m)
{
    v_matrix_t mx_vmirror, mx_scale;
    lv_image_dsc_t *imdsc = (lv_image_dsc_t *)lv_image_get_src(image);
    int16_t w;
    w = lv_obj_get_width(canvas);
    int32_t ox = (w - imdsc->header.w) / 2;
    int32_t oy = 0;
    mx_vmirror = (v_matrix_t){.elm = {{1.0f, 0.0f, 0.0f},
                                      {0.0f, -1.0f, imdsc->header.h}}};
    set_scale_translate_matrix(&mx_scale, g_scale, ox, oy);
    matrix_multiply(m, &mx_vmirror, &mx_scale);
}

static void set_scale_translate_matrix(v_matrix_t *m, float scale, float ox, float oy)
{
    m->elm[0][0] = scale;
    m->elm[0][1] = 0.0f;
    m->elm[0][2] = ox;

    m->elm[1][0] = 0.0f;
    m->elm[1][1] = scale;
    m->elm[1][2] = oy;
}

static void v_add_inklist(v_annot_t *annot)
{
    v_inklist_t *il = annot->data.inklist;
    int16_t w, h;
    w = lv_obj_get_width(canvas);
    h = lv_obj_get_height(canvas);

    annot->pdf_annot_obj = (void *)lv_canvas_create(canvas);
    lv_draw_buf_t *d = malloc(sizeof(lv_draw_buf_t) + w * h * 4);
    lv_obj_t *a = annot->pdf_annot_obj;
    lv_draw_buf_init(d, w, h, LV_COLOR_FORMAT_ARGB8888, w * 4, &d[1], w * h * 4);
    lv_draw_buf_set_flag(d, LV_IMAGE_FLAGS_MODIFIABLE);
    lv_canvas_set_draw_buf(a, d);
    lv_canvas_fill_bg(a, lv_color_hex3(0xccc), LV_OPA_TRANSP);
    lv_layer_t l;

    lv_obj_set_size(a, w, h);

    lv_canvas_init_layer(a, &l);
    d("argb:%02x%02x%02x%02x", il->pen.color.c.alpha, il->pen.color.c.red, il->pen.color.c.green, il->pen.color.c.blue);
    init_rect(&annot->rect);

    d("matrix:%.02f,%.02f,%02f %.02f,%.02f,%.02f", annot->matrix.elm[0][0], annot->matrix.elm[0][1], annot->matrix.elm[0][2], annot->matrix.elm[1][0], annot->matrix.elm[1][1], annot->matrix.elm[1][2]);

    for (int i = 0; i < il->num; i++)
    {
        v_stroke_t *st = &il->strokes[i];
        static lv_draw_line_dsc_t _dsc;
        lv_draw_line_dsc_t *dsc = &_dsc;
        lv_draw_line_dsc_init(dsc);

        d("st->num:%d", st->num);
        for (int j = 1; j < st->num; j++)
        {
            dsc->p1.x = st->points[j - 1].x;
            dsc->p1.y = st->points[j - 1].y;
            dsc->p2.x = st->points[j].x;
            dsc->p2.y = st->points[j].y;
            //            d("orig %p %.02f,%.02f-%.02f,%.02f", st, dsc->p1.x, dsc->p1.y, dsc->p2.x, dsc->p2.y);

            lv_point_precise_t p1 = dsc->p1;
            lv_point_precise_t p2 = dsc->p2;

            dsc->p1.x = annot->matrix.elm[0][0] * p1.x +
                        annot->matrix.elm[0][1] * p1.y +
                        annot->matrix.elm[0][2];
            dsc->p1.y = annot->matrix.elm[1][0] * p1.x +
                        annot->matrix.elm[1][1] * p1.y +
                        annot->matrix.elm[1][2];

            dsc->p2.x = annot->matrix.elm[0][0] * p2.x +
                        annot->matrix.elm[0][1] * p2.y +
                        annot->matrix.elm[0][2];
            dsc->p2.y = annot->matrix.elm[1][0] * p2.x +
                        annot->matrix.elm[1][1] * p2.y +
                        annot->matrix.elm[1][2];

            if (j == 1)
            {
                annot->rect.left = min(annot->rect.left, dsc->p1.x);
                annot->rect.top = min(annot->rect.top, dsc->p1.y);
                annot->rect.right = max(annot->rect.right, dsc->p1.x);
                annot->rect.bottom = max(annot->rect.bottom, dsc->p1.y);
            }
            annot->rect.left = min(annot->rect.left, dsc->p2.x);
            annot->rect.top = min(annot->rect.top, dsc->p2.y);
            annot->rect.right = max(annot->rect.right, dsc->p2.x);
            annot->rect.bottom = max(annot->rect.bottom, dsc->p2.y);

            dsc->color.red = il->pen.color.c.red;
            dsc->color.green = il->pen.color.c.green;
            dsc->color.blue = il->pen.color.c.blue;
            dsc->opa = il->pen.color.c.alpha;
            dsc->width = il->pen.size;
            //            d("%d: argb:%d,%d,%d,%d w:%d %.02f,%.02f-%.02f,%.02f", j, dsc->opa, dsc->color.red, dsc->color.green, dsc->color.blue, dsc->width, dsc->p1.x, dsc->p1.y, dsc->p2.x, dsc->p2.y);
            lv_draw_line(&l, dsc);
        }
    }
    lv_canvas_finish_layer(a, &l);
    d("rect: (%d,%d)-(%d,%d)", annot->rect.left, annot->rect.top, annot->rect.right, annot->rect.bottom);
}

static void v_add_annot(v_annot_t *annot)
{
    d("annot:%p %d", annot, annot->kind);
    ERR_RETn(!annot);

    void (*func[])(v_annot_t *) = {
        (void (*)(v_annot_t *))v_add_inklist,
        (void (*)(v_annot_t *))v_add_freetext,
    };

    TAILQ_INSERT_TAIL(an_head, annot, entry);
    d("an_head %p", an_head->tqh_first);
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
    TAILQ_REMOVE(an_head, annot, entry);
    lv_obj_delete(annot->pdf_annot_obj);
    annot->pdf_annot_obj = NULL;
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

static void draw_annot(const v_draw_event_t *e)
{
    d("");
}
static void draw_erase(const v_draw_event_t *e)
{
    d("");
}
static void show_all(const v_draw_event_t *e)
{
    d("");
    v_set_annot_visibility(false);
}
static void hide_all(const v_draw_event_t *e)
{
    d("");
    v_set_annot_visibility(true);
}

static void draw_user(const v_draw_event_t *e)
{
    d("");
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
    d("an_head: %p", an_head->tqh_first);
    mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    th = malloc(sizeof(pthread_t));
    draw_func_init();
    running = true;
    pthread_create(th, NULL, draw_main, NULL);
}

static void font_load(void)
{
}

static void free_annot_canvas(void *p)
{
    ERR_RETn(!p);
    lv_obj_t *c = (lv_obj_t *)p;

    lv_draw_buf_t *buf = lv_canvas_get_draw_buf(c);

    if (buf)
        free(buf);
    lv_obj_delete(c);

error_return:
    return;
}

static void annot_control_init(void)
{
    annot_control = &_annot_control;
    annot_control->annot = NULL;
    annot_control->remove_button = lv_image_create(canvas);
    annot_control->resize_button = lv_image_create(canvas);
    const lv_image_dsc_t *eraser_img = get_icon_dsc("eraser");
    const lv_image_dsc_t *resize_img = get_icon_dsc("resize");

    lv_image_set_src(annot_control->remove_button, eraser_img);
    lv_image_set_src(annot_control->resize_button, resize_img);
    lv_obj_add_flag(annot_control->remove_button, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(annot_control->resize_button, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event(annot_control->remove_button, on_remove_pressed, LV_EVENT_SINGLE_CLICKED, NULL);
    lv_obj_add_event(annot_control->resize_button, on_resize_dragged, LV_EVENT_ALL, NULL);

    v_annot_set_free_func(free_annot_canvas);
}
static v_status_t v_init_canvas(lv_obj_t *parent)
{
    v_status_t status = ST_SUCCESS;

    ERR_RETn(image);

    status = ST_CREATE_CANVAS_FAILED;

    g_dsc = lv_malloc(sizeof(lv_img_dsc_t));
    ERR_RET(!g_dsc, "malloc");
    memset(g_dsc, 0, sizeof(lv_img_dsc_t));
    image = lv_img_create(parent);
    ERR_RET(!image, "lv_img_create");

    lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 0);

    canvas = lv_canvas_create(parent);
    ERR_RET(!canvas, "lv_canvas_create");

    canvas_buf = malloc(sizeof(lv_draw_buf_t) + WIDTH * HEIGHT * 4);
    ERR_RET(!canvas_buf, "malloc");
    lv_draw_buf_init(canvas_buf, WIDTH, HEIGHT, LV_COLOR_FORMAT_ARGB8888, WIDTH * 4, &canvas_buf[1], WIDTH * HEIGHT * 4);
    lv_draw_buf_set_flag(canvas_buf, LV_IMAGE_FLAGS_MODIFIABLE);

    lv_canvas_set_draw_buf(canvas, canvas_buf);
    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_TRANSP);
    lv_obj_center(canvas);

    font_load();
    draw_thread_init();

    annot_control_init();

    status = ST_SUCCESS;
error_return:
    return status;
}

static void reset_annotation(void)
{
    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_TRANSP);
}

static uint8_t *autoscale(uint8_t *orig, int *_w, int *_h, float *_scale)
{
    uint8_t *data = NULL;
    int w = *_w;
    int h = *_h;
    float scale_x = (float)lv_obj_get_width(lv_screen_active()) / w;
    float scale_y = (float)lv_obj_get_height(lv_screen_active()) / h;

    float scale = min(scale_x, scale_y);
    *_scale = scale;

    int scaled_w = (int)((float)w * scale);
    int scaled_h = (int)((float)h * scale);
    d("scale %.02f,%.02f -> %.02f (%d, %d) => (%d, %d)", scale_x, scale_y, scale, w, h, scaled_w, scaled_h);

    static cairo_surface_t *scaled_surface = NULL;

    cairo_surface_t *orig_surface = cairo_image_surface_create_for_data(orig, CAIRO_FORMAT_ARGB32, w, h, w * 4);
    ERR_RET(!orig_surface, "cairo_image_surface_create_for_data");
    //    if (scaled_surface)
    //    {
    //        cairo_surface_destroy(scaled_surface);
    //    }
    scaled_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, scaled_w, scaled_h);

    cairo_t *cr = cairo_create(scaled_surface);
    cairo_scale(cr, (double)scale, (double)scale);
    cairo_set_source_surface(cr, orig_surface, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(orig_surface);

    *_w = scaled_w;
    *_h = scaled_h;
    data = cairo_image_surface_get_data(scaled_surface);
error_return:
    return data;
}

static void clear_annots(void)
{
    v_annot_t *iter;
    iter = TAILQ_FIRST(an_head);
    while (iter)
    {
        d("iter:%p head:%p", iter, an_head->tqh_first);
        TAILQ_REMOVE(an_head, iter, entry);
        v_remove_annot(iter);
        v_free_annot(iter);
        iter = TAILQ_FIRST(an_head);
    }
}
static v_status_t v_show_image(v_image_t *im)
{
    clear_annots();
    //    int w, h;
    //    float scale, scale_x, scale_y;
    if (g_dsc->data)
    {
        lv_free((uint8_t *)g_dsc->data);
    }
    reset_annotation();

    int w = im->width;
    int h = im->height;
    float scale;
    uint8_t *data = autoscale(im->buf, &w, &h, &scale);

    g_scale = scale;

    g_dsc->data = data;
    g_dsc->header.cf = LV_COLOR_FORMAT_ARGB8888;
    g_dsc->header.w = w;
    g_dsc->header.h = h;
    g_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    g_dsc->data_size = w * h * 4;
    lv_img_set_src(image, g_dsc);
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

static void show_annot_control(v_annot_t *a)
{
    lv_obj_t *r, *d;
    r = annot_control->resize_button;
    d = annot_control->remove_button;

    float x = a->rect.right;
    float y = a->rect.bottom;

    lv_obj_set_pos(r, x, y);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_HIDDEN);

    // 2個目も同様に +40 の処理を行う（Y方向にシフト）
    x = a->rect.right;
    y = a->rect.bottom + 40;

    lv_obj_set_pos(d, x, y);
    lv_obj_remove_flag(d, LV_OBJ_FLAG_HIDDEN);
}

static void init_rect(v_rect_t *rect)
{
    ERR_RET(!rect, "invalid arg");
    rect->left = INT16_MAX;
    rect->top = INT16_MAX;
    rect->right = INT16_MIN;
    rect->bottom = INT16_MIN;
error_return:
    return;
}

static void update_rect_point(v_rect_t *r, v_point_t *p)
{
    if (p->x < r->left)
        r->left = p->x;
    if (p->x > r->right)
        r->right = p->x;
    if (p->y < r->top)
        r->top = p->y;
    if (p->y < r->bottom)
        r->bottom = p->y;
}

static void update_rect(v_annot_t *a)
{
    ERR_RETn(!a);
    if (a->kind == V_ANNOT_INKLIST)
    {
        v_inklist_t *il = a->data.inklist;
        ERR_RETn(!il);
        init_rect(&a->rect);
        for (int i = 0; i < il->num; i++)
        {
            v_stroke_t *st = &il->strokes[i];
            if (!st)
                continue;
            for (int j = 0; j < st->num; j++)
            {
                v_point_t *p = &st->points[j];
                static v_point_t c;
                c.x = a->matrix.elm[0][0] * p->x +
                      a->matrix.elm[0][1] * p->y +
                      a->matrix.elm[0][2];
                c.y = a->matrix.elm[1][0] * p->x +
                      a->matrix.elm[1][1] * p->y +
                      a->matrix.elm[1][2];

                update_rect_point(&a->rect, &c);
            }
        }
    }
error_return:
    return;
}
static void v_select(lv_point_t *pos)
{
    v_annot_t *annot = NULL, *iter;
    TAILQ_FOREACH(iter, an_head, entry)
    {
        // update_rect(iter);
        if (in_rect(pos, &iter->rect))
        {
            // clang-format off
            d("%d,%d in (%d,%d)-(%d,%d)"
                , pos->x, pos->y
                , iter->rect.left
                , iter->rect.top
                , iter->rect.right
                , iter->rect.bottom);
            // clang-format on
            annot = iter;
            break;
        }
    }
    ERR_RETn(!annot);

    annot_control->annot = annot;
    d("target:%p %d", annot, annot->kind);

    show_annot_control(annot);

error_return:
    return;
}
static void set_move_matrix(v_matrix_t *m, const lv_point_t *from, const lv_point_t *to)
{
    float dx = (float)(to->x - from->x);
    float dy = (float)(to->y - from->y);

    m->elm[0][0] = 1.0f; // a
    m->elm[0][1] = 0.0f; // c
    m->elm[0][2] = dx;   // e

    m->elm[1][0] = 0.0f; // b
    m->elm[1][1] = 1.0f; // d
    m->elm[1][2] = dy;   // f
}

static void v_move(const lv_point_t *from, const lv_point_t *to)
{
    v_annot_t *a = annot_control->annot;
    v_annot_t *prev, *next;
    prev = v_clone_annot(a);
    v_remove_annot(a);
    v_matrix_t matrix;

    set_move_matrix(&matrix, from, to);
    matrix_add(&a->matrix, &matrix);
    // d(" M1:\n%s", dump_matrix(&before_matrix));
    // d(" M2:\n%s", dump_matrix(&after_matrix));
    // d(" M1*M2:\n%s", dump_matrix(&a->matrix));
    if (a->kind == V_ANNOT_INKLIST && a->data.inklist->coord_type == V_ANNOT_COORD_ORIGINAL)
    {
        a->data.inklist->coord_type = V_ANNOT_COORD_MODIFIED;
    }
    next = v_clone_annot(a);
    undo_push(prev, next, UNDO_ACTION_TRANSFORM_INK);
    v_add_annot(a);
    hide_annot_control();
    show_annot_control(a);
}

// static void call_remove_dialog(void)
//{
// }
static void hide_annot_control(void)
{
    lv_obj_t *d = annot_control->remove_button;
    lv_obj_t *r = annot_control->resize_button;
    lv_obj_add_flag(d, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
}
static void remove_callback(bool result)
{
    d("result: %d", result);
    ERR_RETn(!result);

    hide_annot_control();
    v_annot_t *a = annot_control->annot;
    v_annot_t *prev = v_clone_annot(a);
    annot_control->annot = NULL;
    undo_push(prev, NULL, UNDO_ACTION_DELETE_INK);

    v_remove_annot(a);
    // TODO いつ解放するか

error_return:
    return;
}
static void on_remove_pressed(lv_event_t *ev)
{
    d("");
    // lv_async_call((lv_async_cb_t)call_remove_dialog, NULL);
    show_modal_dialog(lv_screen_active(), "Delete this item ?", remove_callback);
}

static void set_scale_matrix(v_matrix_t *m, const lv_point_precise_t *center, float scale)
{
    m->elm[0][0] = scale;
    m->elm[0][1] = 0.0f;
    m->elm[0][2] = center->x * (1.0f - scale);

    m->elm[1][0] = 0.0f;
    m->elm[1][1] = scale;
    m->elm[1][2] = center->y * (1.0f - scale);
}

static void on_resize_dragged(lv_event_t *ev)
{
    lv_event_code_t code = lv_event_get_code(ev);
    if (code > LV_EVENT_RELEASED)
        return;

    lv_indev_t *indev = lv_event_get_indev(ev);
    lv_point_t point;
    static lv_point_precise_t center;
    static float d;
    //    static float original_scale;
    lv_indev_get_point(indev, &point);
    d("%d: (%d,%d)", code, point.x, point.y);

    if (code == LV_EVENT_PRESSED)
    {
        lv_obj_t *btn = lv_event_get_target_obj(ev);
        lv_obj_set_style_image_opa(btn, LV_OPA_50, 0);
        v_annot_t *a = annot_control->annot;
        center = rect_center(&a->rect, NULL);
        d = distancef(&center, &point);
        // original_scale = scale_factor(&a->matrix);
    }
    else if (code == LV_EVENT_PRESSING)
    {
    }
    else if (code == LV_EVENT_RELEASED)
    {
        lv_obj_t *btn = lv_event_get_target_obj(ev);
        lv_obj_set_style_image_opa(btn, LV_OPA_COVER, 0);
        float after = distancef(&center, &point);
        d("os:%.02f od:%.02f ad:%.02f", d, after);
        float scale = after / d;
        v_annot_t *a = annot_control->annot;
        d("target:%p", a);
        v_annot_t *prev, *next;
        prev = v_clone_annot(a);
        v_remove_annot(a);
        v_matrix_t before_matrix, after_matrix;
        memcpy(&before_matrix, &a->matrix, sizeof(v_matrix_t));
        set_scale_matrix(&after_matrix, &center, scale);
        //        d("c:%f,%f s:%.02f, aft:%.02f,%.02f,%.02f %.02f,%.02f,%.02f", center.x, center.y, scale, after_matrix.elm[0][0], after_matrix.elm[0][1], after_matrix.elm[0][2], after_matrix.elm[1][0], after_matrix.elm[1][1], after_matrix.elm[1][2]);
        matrix_multiply(&a->matrix, &after_matrix, &before_matrix);
        //        d(" M1:\n%s", dump_matrix(&before_matrix));
        //        d(" M2:\n%s", dump_matrix(&after_matrix));
        //        d(" M1*M2:\n%s", dump_matrix(&a->matrix));
        if (a->kind == V_ANNOT_INKLIST && a->data.inklist->coord_type == V_ANNOT_COORD_ORIGINAL)
        {
            a->data.inklist->coord_type = V_ANNOT_COORD_MODIFIED;
        }
        next = v_clone_annot(a);
        undo_push(prev, next, UNDO_ACTION_TRANSFORM_INK);
        v_add_annot(a);
        hide_annot_control();
        show_annot_control(a);
    }
}

static v_annots_t *v_all_annots(void)
{
    v_annots_t *ret = NULL;
    v_annots_t *annots;
    v_annot_t *iter;
    int i, n;

    n = 0;
    TAILQ_FOREACH(iter, an_head, entry)
    {
        n++;
    }

    annots = malloc(sizeof(v_annots_t) + sizeof(v_annot_t *) * n);
    ERR_RET(!annots, "memory error");

    annots->num = n;

    i = 0;
    TAILQ_FOREACH(iter, an_head, entry)
    {
        annots->annot[i++] = iter;
    }

    ret = annots;
error_return:
    return ret;
}