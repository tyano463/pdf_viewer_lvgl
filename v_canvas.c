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

typedef struct str_point
{
    TAILQ_ENTRY(str_point)
    entry;
    float x;
    float y;
    v_pen_draw_mode_t mode;
    lv_vector_path_t *stroke;
} point_t;

TAILQ_HEAD(tq_head, str_point)
_head;
struct tq_head *head;

static void *draw_main(void *);

static void draw_thread_init(void)
{
    head = &_head;
    TAILQ_INIT(head);

    mutex = lv_malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    th = lv_malloc(sizeof(pthread_t));
    running = true;
    pthread_create(th, NULL, draw_main, NULL);
}

v_status_t show_canvas(lv_obj_t *parent)
{
    canvas = lv_canvas_create(parent);
    LV_DRAW_BUF_INIT_STATIC(canvas_buf);

    lv_canvas_set_draw_buf(canvas, &canvas_buf);
    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_TRANSP);
    lv_obj_center(canvas);

    draw_thread_init();

    d("canvas:%p", canvas);
    return ST_SUCCESS;
}

point_t *last_point;
void draw_line_to(float x, float y, v_pen_draw_mode_t mode)
{
    point_t *point = lv_malloc(sizeof(point_t));
    static int cnt;
    if (mode == V_PEN_DRAW_START)
    {
        cnt = 0;
        point->x = x;
        point->y = y;
        point->stroke = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
        point->mode = mode;
        last_point = point;
    }
    else
    {
        point->x = x;
        point->y = y;
        point->mode = mode;
        point->stroke = last_point->stroke;
    }
    pthread_mutex_lock(mutex);
    TAILQ_INSERT_TAIL(head, point, entry);
    pthread_mutex_unlock(mutex);
    d("add %p (%f, %f) %d", point, x, y, mode);
}

static void *draw_main(void *arg)
{
    static lv_vector_path_t *current;
//    static lv_vector_dsc_t *dsc;
    static lv_draw_line_dsc_t *dsc;
    dsc = lv_malloc(sizeof(lv_draw_line_dsc_t));
    while (running)
    {
        pthread_mutex_lock(mutex);

        if (head->tqh_first)
        {
            point_t *point = head->tqh_first;
            TAILQ_REMOVE(head, point, entry);
            pthread_mutex_unlock(mutex);
            d("fetch %p mode:%d (%.1f,%.1f)", point, point->mode, point->x, point->y);
            lv_fpoint_t fp = {
                .x = point->x,
                .y = point->y,
            };
            if (point->mode == V_PEN_DRAW_START)
            {
                lv_draw_line_dsc_init(dsc);
                dsc->p1.x = fp.x;
                dsc->p1.y = fp.y;
                lv_canvas_init_layer(canvas, &layer);
                d("dsc:%p path:%p", dsc, point->stroke);
            }
            else if (point->mode == V_PEN_DRAW_END)
            {
                // lv_vector_dsc_delete(dsc);
                d("dsc:%p path:%p", dsc, point->stroke);
                dsc->p2.x = fp.x;
                dsc->p2.y = fp.y;
                lv_draw_line(&layer, dsc);
                lv_canvas_finish_layer(canvas, &layer);
            }
            else
            {
                dsc->p2.x = fp.x;
                dsc->p2.y = fp.y;
                lv_draw_line(&layer, dsc);

                lv_draw_line_dsc_init(dsc);
                dsc->p1.x = fp.x;
                dsc->p1.y = fp.y;
                d("dsc:%p path:%p", dsc, point->stroke);
            }

            point->stroke = current;
            lv_task_handler();
        }
        else
        {
            pthread_mutex_unlock(mutex);
            usleep(10000);
        }
    }
    return NULL;
}