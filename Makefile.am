bin_PROGRAMS = pdf_viewer_lvgl
PATCH_FILE = lvgl.patch
PATCH_STAMP = lvgl/.patched
AUTOMAKE_OPTIONS = subdir-objects

LVGL_SOURCES = \
$(shell find lvgl/src -name "*.c") \
$(shell find lvgl/src -name "*.cpp")

STATIC_LIB = liblvgl.a

noinst_LIBRARIES = liblvgl.a
liblvgl_a_SOURCES = dummy.cpp $(LVGL_SOURCES)
am_liblvgl_a_OBJECTS = $(LVGL_SOURCES:.c=.o) $(LVGL_SOURCES:.cpp=.o)

pdf_viewer_lvgl_LDADD = $(STATIC_LIB)

pdf_viewer_lvgl_SOURCES = src/main.c \
	src/v_mouse_cursor.c \
	src/v_menu.c \
	src/v_colorpicker.c \
	src/v_pdf.c \
	src/v_log.c \
	src/v_file.c \
	src/v_misc.c \
	src/v_color.c \
	src/v_icon.c \
	src/v_assets_list.c \
	src/v_canvas.c \
	src/v_settings.c \
	src/v_pen.c \
	src/v_slider.c \
	src/v_png.c \
	src/v_midi.c \
	src/v_musicxml.c \
	fonts/source_hans_16.c \
	fonts/source_hans_20.c \
	fonts/source_hans_24.c \
	src/v_core.c \
	src/convert.c \
	src/v_svg.c

C_INC = \
	-Isrc \
    -I$(srcdir)/lvgl \
    -I$(srcdir)/lvgl/src

AM_CFLAGS = $(C_INC) -g -O0 -DHIDE_HIDDEN_FILES=1 `pkg-config librsvg-2.0 cairo libcjson libjpeg --cflags` -Wall
AM_CXXFLAGS = $(C_INC) -g -O0 -DHIDE_HIDDEN_FILES=1 `pkg-config librsvg-2.0 cairo libcjson libjpeg --cflags` 
#AM_CFLAGS = $(C_INC) -g -O0 -fsanitize=address
#AM_CPPFLAGS = $(C_INC) -g -O0 -fsanitize=address
AM_LDFLAGS = -lmupdf -lX11 -lstdc++ -lm -lsmf -Wl,-rpath=/usr/local/lib `pkg-config librsvg-2.0 cairo libcjson libjpeg --libs`

ASSETS = hamburger pen close_button eraser resize \
	open_book select circle \
	prev_page next_page
ASSET_SRCS = $(addprefix assets_, $(addsuffix _bmp.c, $(ASSETS)))
JSON = menu ja en
JSON_SRCS = $(addprefix assets_, $(addsuffix _json.c, $(JSON)))

pdf_viewer_lvgl_SOURCES += $(ASSET_SRCS) $(JSON_SRCS)
#BUILT_SOURCES = $(pdf_viewer_lvgl_SOURCES)

EXTRA_DIST = assets/hamburger.bmp \
	assets/pen.bmp \
	assets/close_button.bmp \
	assets/open_book.bmp \
	assets/eraser.bmp \
	assets/resize.bmp \
	assets/select.bmp \
	assets/circle.bmp \
	assets/prev_page.bmp \
	assets/next_page.bmp \
	assets/menu.json \
	assets/ja.json \
	assets/en.json

SOURCES = $(pdf_viewer_lvgl_SOURCES)
am_pdf_viewer_lvgl_OBJECTS = $(PATCH_STAMP) $(pdf_viewer_lvgl_SOURCES:.c=.o)

$(PATCH_STAMP):
	@echo "Applying patch to lvgl..."
	patch -d lvgl -p1 < $(PATCH_FILE) && touch $(PATCH_STAMP)

pdf_viewer_lvgl_LDADD = liblvgl.a
$(STATIC_LIB): $(PATCH_STAMP) $(liblvgl_a_OBJECTS)
	$(AR) rcs $(STATIC_LIB) $(liblvgl_a_OBJECTS)

assets_%_bmp.c: assets/%.bmp
	xxd -i $< > $@

assets_%_json.c: assets/%.json
	xxd -i $< > $@

