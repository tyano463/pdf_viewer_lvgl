#include "v_core.h"
#include <math.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <librsvg/rsvg.h>
#include <jpeglib.h>
#include "v_misc.h"

#define A4_WIDTH_PT 595.0
#define A4_HEIGHT_PT 842.0

#define EVENTFD_PATH "/tmp/ipc_fifo"

static int fd_in;

float distance(const lv_point_t *a, const lv_point_t *b)
{
    float d2 = (b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y);
    return sqrtf(d2);
}

float distancef(const lv_point_precise_t *a, const lv_point_t *b)
{
    float d2 = (b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y);
    return sqrtf(d2);
}

bool in_rect(lv_point_t *a, v_rect_t *rect)
{
    bool ret = false;
    ERR_RET(!a || !rect, "point:%p, rect:%p", a, rect);

    // clang-format off
    ret = rect->left <= a->x && a->x <= rect->right 
        && rect->top <= a->y && a->y <= rect->bottom;
    // clang-format on

error_return:
    return ret;
}

static void modal_btn_event_cb(lv_event_t *e)
{
    const lv_obj_t *btn = lv_event_get_target(e);
    const char *btn_text = lv_label_get_text(lv_obj_get_child(btn, 0));
    v_message_callback_t callback = lv_event_get_user_data(e);
    bool result;

    result = strcmp(btn_text, "Yes") == 0;
    lv_obj_delete(lv_obj_get_parent(lv_obj_get_parent(lv_obj_get_parent(btn))));
    if (callback)
        callback(result);
}

void show_modal_dialog(lv_obj_t *parent, const char *title, v_message_callback_t callback)
{
    lv_obj_t *modal_bg = lv_obj_create(parent);
    lv_obj_remove_style_all(modal_bg);
    lv_obj_set_size(modal_bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);
    lv_obj_center(modal_bg);

    lv_obj_t *dialog = lv_obj_create(modal_bg);
    lv_obj_remove_style_all(dialog);
    lv_obj_set_size(dialog, 200, 120);
    lv_obj_center(dialog);
    lv_obj_set_style_border_width(dialog, 1, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex3(0x888), 0);
    lv_obj_set_style_shadow_width(dialog, 20, 0);
    lv_obj_set_style_bg_color(dialog, (lv_color_t){255, 255, 255}, 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dialog, 10, 0);

    lv_obj_t *label = lv_label_create(dialog);
    lv_obj_remove_style_all(label);
    lv_label_set_text(label, title);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *btn_row = lv_obj_create(dialog);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(btn_row, 200, 40);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_pad_gap(btn_row, 20, 0);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);

    lv_obj_t *btn_yes = lv_btn_create(btn_row);
    lv_obj_t *label_yes = lv_label_create(btn_yes);
    lv_obj_remove_style_all(btn_yes);
    lv_obj_remove_style_all(label_yes);
    lv_label_set_text(label_yes, "Yes");
    lv_obj_set_size(btn_yes, 80, 30);
    lv_obj_center(label_yes);
    lv_obj_set_style_border_width(btn_yes, 1, 0);
    lv_obj_set_style_shadow_width(btn_yes, 20, 0);
    lv_obj_set_style_border_color(btn_yes, lv_color_hex3(0x888), 0);
    lv_obj_set_style_radius(btn_yes, 10, 0);
    lv_obj_add_event_cb(btn_yes, modal_btn_event_cb, LV_EVENT_CLICKED, callback);

    lv_obj_t *btn_no = lv_btn_create(btn_row);
    lv_obj_t *label_no = lv_label_create(btn_no);
    lv_obj_remove_style_all(btn_no);
    lv_obj_remove_style_all(label_no);
    lv_obj_set_size(btn_no, 80, 30);
    lv_label_set_text(label_no, "No");
    lv_obj_center(label_no);
    lv_obj_set_style_border_width(btn_no, 1, 0);
    lv_obj_set_style_shadow_width(btn_no, 20, 0);
    lv_obj_set_style_border_color(btn_no, lv_color_hex3(0x888), 0);
    lv_obj_set_style_radius(btn_no, 10, 0);
    lv_obj_add_event_cb(btn_no, modal_btn_event_cb, LV_EVENT_CLICKED, callback);
}

lv_point_precise_t rect_center(const v_rect_t *p, const v_matrix_t *m)
{
    lv_point_precise_t center = {0.0f, 0.0f};

    if (p)
    {
        center.x = (p->left + p->right) / 2.0f;
        center.y = (p->top + p->bottom) / 2.0f;

        if (m)
        {
            lv_point_precise_t transformed;
            transformed.x = m->elm[0][0] * center.x + m->elm[0][1] * center.y + m->elm[0][2];
            transformed.y = m->elm[1][0] * center.x + m->elm[1][1] * center.y + m->elm[1][2];

            center = transformed;
        }
    }

    return center;
}

float scale_factor(const v_matrix_t *m)
{
    if (!m)
        return 1.0f;

    float scale_x = sqrtf(m->elm[0][0] * m->elm[0][0] + m->elm[0][1] * m->elm[0][1]);
    float scale_y = sqrtf(m->elm[1][0] * m->elm[1][0] + m->elm[1][1] * m->elm[1][1]);

    return (scale_x + scale_y) / 2.0f;
}

bool solve_matrix(v_matrix_t *m, const v_matrix_t *a, const v_matrix_t *b)
{
    if (!a || !b || !m)
        return false;

    float x0 = a->elm[0][0], y0 = a->elm[0][1];
    float x1 = a->elm[1][0], y1 = a->elm[1][1];

    float x0p = b->elm[0][0], y0p = b->elm[0][1];
    float x1p = b->elm[1][0], y1p = b->elm[1][1];

    // 6元連立方程式を解く（クラメルの公式で解けるがここでは直接式展開）

    float det = (x0 * y1 - x1 * y0);
    if (det == 0.0f)
        return false; // 解なし

    float inv_det = 1.0f / det;

    // 線形部分
    m->elm[0][0] = (x0p * y1 - x1p * y0) * inv_det;
    m->elm[0][1] = (-x0p * x1 + x1p * x0) * inv_det;

    m->elm[1][0] = (y0p * y1 - y1p * y0) * inv_det;
    m->elm[1][1] = (-y0p * x1 + y1p * x0) * inv_det;

    // 並進（平行移動）項を計算（a × m + 並進 = b より）
    m->elm[0][2] = x0p - (x0 * m->elm[0][0] + y0 * m->elm[0][1]);
    m->elm[1][2] = y0p - (x0 * m->elm[1][0] + y0 * m->elm[1][1]);

    return true;
}

void matrix_multiply(v_matrix_t *result, v_matrix_t *m1, v_matrix_t *m2)
{
    if (!result || !m1 || !m2)
        return;

    v_matrix_t temp;

    // 行列の積を計算 (2×3 行列の乗算)
    temp.elm[0][0] = m1->elm[0][0] * m2->elm[0][0] + m1->elm[0][1] * m2->elm[1][0];
    temp.elm[0][1] = m1->elm[0][0] * m2->elm[0][1] + m1->elm[0][1] * m2->elm[1][1];
    temp.elm[0][2] = m1->elm[0][0] * m2->elm[0][2] + m1->elm[0][1] * m2->elm[1][2] + m1->elm[0][2];

    temp.elm[1][0] = m1->elm[1][0] * m2->elm[0][0] + m1->elm[1][1] * m2->elm[1][0];
    temp.elm[1][1] = m1->elm[1][0] * m2->elm[0][1] + m1->elm[1][1] * m2->elm[1][1];
    temp.elm[1][2] = m1->elm[1][0] * m2->elm[0][2] + m1->elm[1][1] * m2->elm[1][2] + m1->elm[1][2];

    // 結果を返す
    *result = temp;
}

v_status_t v_init_external_receiver(void)
{
    fd_in = 0;
    int ret;
    ret = mkfifo(EVENTFD_PATH, 0666);
    ERR_RET(ret < 0 && errno != EEXIST, "fd create fail");

    fd_in = open(EVENTFD_PATH, O_RDONLY | O_NONBLOCK);

error_return:
    return fd_in >= 0 ? ST_SUCCESS : ST_EXT_RECEIVER_INIT_FAILED;
}

v_status_t v_check_external_command(v_ext_command_t *ext_command)
{
    v_status_t status = ST_EXT_RECEIVER_INIT_FAILED;
    ERR_RET(fd_in <= 0, "efd open failed");

    ssize_t n = read(fd_in, ext_command, sizeof(v_ext_command_t));

    if (n > 0)
    {
        status = ST_SUCCESS;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        status = ST_EXT_NO_RECEIVE;
    }
error_return:
    return status;
}

bool inverse_matrix(const v_matrix_t *orig, v_matrix_t *inv)
{
    // アフィン変換行列の要素を抽出
    float a = orig->elm[0][0];
    float c = orig->elm[0][1];
    float e = orig->elm[0][2];
    float b = orig->elm[1][0];
    float d = orig->elm[1][1];
    float f = orig->elm[1][2];

    // 行列の 2x2 部分の行列式を計算
    float det = a * d - b * c;
    if (det == 0.0f)
        return false; // 逆行列が存在しない

    float inv_det = 1.0f / det;

    // 逆行列を計算
    inv->elm[0][0] = d * inv_det;
    inv->elm[0][1] = -c * inv_det;
    inv->elm[0][2] = (c * f - d * e) * inv_det;

    inv->elm[1][0] = -b * inv_det;
    inv->elm[1][1] = a * inv_det;
    inv->elm[1][2] = (b * e - a * f) * inv_det;

    return true;
}

char *svg2pdf(const char *file)
{
    char *pdf_path = NULL;
    char *output;
    GError *error = NULL;
    RsvgHandle *rsvg_handle = rsvg_handle_new_from_file(file, &error);
    ERR_RET(!rsvg_handle, "rsvg_handle_new_from_file @ %s", file);

    gdouble w, h;
    rsvg_handle_get_intrinsic_size_in_pixels(rsvg_handle, &w, &h);
    RsvgRectangle viewport = {
        .x = 0.0,
        .y = 0.0,
        .width = w,
        .height = h};

    output = strdup(file);
    ERR_RET(!output, "strdup");
    ERR_RET(!rename_ext(output, "pdf"), "rename ext");

    cairo_surface_t *surface = cairo_pdf_surface_create(output, w, h);
    cairo_t *cr = cairo_create(surface);
    rsvg_handle_render_document(rsvg_handle, cr, &viewport, &error);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(rsvg_handle);

    pdf_path = output;
error_return:
    return pdf_path;
}

char *png2pdf(const char *file)
{
    char *ret = NULL;
    ERR_RET(!file, "png file is null");
    char *pdf_file = strdup(file);
    ERR_RET(!pdf_file, "strdup");
    ERR_RET(!rename_ext(pdf_file, "pdf"), "rename_ext");

    cairo_surface_t *pdf = NULL;
    cairo_t *cr = NULL;
    cairo_surface_t *image = NULL;

    image = cairo_image_surface_create_from_png(file);
    ERR_RET(cairo_surface_status(image) != CAIRO_STATUS_SUCCESS, "load png");

    int img_width = cairo_image_surface_get_width(image);
    int img_height = cairo_image_surface_get_height(image);

    pdf = cairo_pdf_surface_create(pdf_file, A4_WIDTH_PT, A4_HEIGHT_PT);
    ERR_RET(cairo_surface_status(pdf) != CAIRO_STATUS_SUCCESS, "create pdf surface");

    cr = cairo_create(pdf);
    ERR_RET(cairo_status(cr) != CAIRO_STATUS_SUCCESS, "create cairo context");

    double scale_x = A4_WIDTH_PT / img_width;
    double scale_y = A4_HEIGHT_PT / img_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;

    double dx = (A4_WIDTH_PT - img_width * scale) / 2.0;
    double dy = (A4_HEIGHT_PT - img_height * scale) / 2.0;

    cairo_translate(cr, dx, dy);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);

    cairo_show_page(cr);

    ret = pdf_file;

error_return:
    if (cr)
        cairo_destroy(cr);
    if (pdf)
        cairo_surface_destroy(pdf);
    if (image)
        cairo_surface_destroy(image);
    if (!ret && pdf_file)
        free(pdf_file);
    return ret;
}
char *jpeg2pdf(const char *file)
{
    char *ret = NULL;
    ERR_RET(!file, "jpeg file is null");
    char *pdf_file = strdup(file);
    ERR_RET(!pdf_file, "strdup");
    ERR_RET(!rename_ext(pdf_file, "pdf"), "rename_ext");

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *infile = NULL;
    JSAMPARRAY buffer = NULL;
    cairo_surface_t *image = NULL, *pdf = NULL;
    cairo_t *cr = NULL;

    // JPEG デコード準備
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    infile = fopen(file, "rb");
    ERR_RET(!infile, "fopen jpeg");
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    J_COLOR_SPACE cs = cinfo.out_color_space;
    ERR_RET(cs != JCS_RGB && cs != JCS_GRAYSCALE && cs != JCS_CMYK && cs != JCS_YCbCr, "color space %d is not supported", cs);

    d("color space:%d", cs);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_stride = width * 4; // CairoはRGBA

    uint8_t *raw_data = calloc(height, row_stride);
    ERR_RET(!raw_data, "calloc");

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width * cinfo.output_components, 1);

    for (int y = 0; y < height; y++)
    {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        for (int x = 0; x < width; x++)
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;

            switch (cinfo.out_color_space)
            {
            case JCS_RGB:
                r = buffer[0][x * cinfo.output_components + 0];
                g = buffer[0][x * cinfo.output_components + 1];
                b = buffer[0][x * cinfo.output_components + 2];
                a = 255;
                break;
            case JCS_GRAYSCALE:
                // グレースケール → R = G = B
                r = g = b = buffer[0][x];
                a = 255;
                break;

            case JCS_CMYK:
                // CMYK → RGB に変換（単純近似: R=255−C, G=255−M, B=255−Y）
                {
                    uint8_t c = buffer[0][x * 4 + 0];
                    uint8_t m = buffer[0][x * 4 + 1];
                    uint8_t y = buffer[0][x * 4 + 2];
                    // uint8_t k = buffer[0][x * 4 + 3];

                    // TODO: なぜか白が255になってる
                    r = c;
                    g = m;
                    b = y;
                    a = 255;
                }
                break;

            case JCS_YCbCr:
                // JPEG の標準カラー空間（YCbCr）→ RGB に変換
                {
                    int y = buffer[0][x * 3 + 0];
                    int cb = buffer[0][x * 3 + 1];
                    int cr = buffer[0][x * 3 + 2];

                    int r_ = y + 1.402 * (cr - 128);
                    int g_ = y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128);
                    int b_ = y + 1.772 * (cb - 128);

                    // clamp to [0, 255]
                    r = (uint8_t)(r_ < 0 ? 0 : r_ > 255 ? 255
                                                        : r_);
                    g = (uint8_t)(g_ < 0 ? 0 : g_ > 255 ? 255
                                                        : g_);
                    b = (uint8_t)(b_ < 0 ? 0 : b_ > 255 ? 255
                                                        : b_);
                    a = 255;
                }
                break;
            default:
                ERR_RET(true, "invalid root");
                break;
            }
            uint8_t *dst = &raw_data[y * row_stride + x * 4];
            dst[0] = b;
            dst[1] = g;
            dst[2] = r;
            dst[3] = a;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    infile = NULL;

    // Cairo image surface 作成（ARGB32）
    image = cairo_image_surface_create_for_data(
        raw_data, CAIRO_FORMAT_RGB24, width, height, row_stride);
    ERR_RET(cairo_surface_status(image) != CAIRO_STATUS_SUCCESS, "cairo image");

    // PDF surface 作成（A4）
    pdf = cairo_pdf_surface_create(pdf_file, A4_WIDTH_PT, A4_HEIGHT_PT);
    ERR_RET(cairo_surface_status(pdf) != CAIRO_STATUS_SUCCESS, "create pdf");

    cr = cairo_create(pdf);

    // スケーリング（A4内に収める）
    double sx = A4_WIDTH_PT / width;
    double sy = A4_HEIGHT_PT / height;
    double scale = sx < sy ? sx : sy;
    double dx = (A4_WIDTH_PT - width * scale) / 2.0;
    double dy = (A4_HEIGHT_PT - height * scale) / 2.0;

    cairo_translate(cr, dx, dy);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, image, 0, 0);
    cairo_paint(cr);
    cairo_show_page(cr);

    ret = pdf_file;

error_return:
    if (cr)
        cairo_destroy(cr);
    if (pdf)
        cairo_surface_destroy(pdf);
    if (image)
        cairo_surface_destroy(image);
    if (raw_data)
        free(raw_data);
    if (infile)
        fclose(infile);
    if (!ret && pdf_file)
        free(pdf_file);
    return ret;
}
