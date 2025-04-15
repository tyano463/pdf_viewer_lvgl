bin_PROGRAMS = pdf_viewer_lvgl
LVGL_SOURCES = \
$(shell find lvgl/src -name "*.c") \
$(shell find lvgl/src -name "*.cpp")

STATIC_LIB = liblvgl.a

noinst_LIBRARIES = liblvgl.a
liblvgl_a_SOURCES = dummy.cpp $(LVGL_SOURCES)
am_liblvgl_a_OBJECTS = $(LVGL_SOURCES:.c=.o) $(LVGL_SOURCES:.cpp=.o)

pdf_viewer_lvgl_LDADD = $(STATIC_LIB)

pdf_viewer_lvgl_SOURCES = main.c \
	v_mouse_cursor.c \
	v_menu.c \
	v_pdf.c \
	v_log.c \
	v_file.c \
	v_misc.c \
	v_icon.c \
	v_canvas.c \
	v_assets_list.c \
	v_pen.c

C_INC = \
    -I$(srcdir)/lvgl \
    -I$(srcdir)/lvgl/src

AM_CFLAGS = $(C_INC) -g -O0 -DHIDE_HIDDEN_FILES=1
AM_CXXFLAGS = $(C_INC) -g -O0 -DHIDE_HIDDEN_FILES=1
#AM_CFLAGS = $(C_INC) -g -O0 -fsanitize=address
#AM_CPPFLAGS = $(C_INC) -g -O0 -fsanitize=address
AM_LDFLAGS = -lmupdf -lX11 -lstdc++ -lm

ASSETS = hamburger pen24
ASSET_SRCS = $(addprefix assets_, $(addsuffix _bmp.c, $(ASSETS)))

pdf_viewer_lvgl_SOURCES += $(ASSET_SRCS)
#BUILT_SOURCES = $(pdf_viewer_lvgl_SOURCES)

EXTRA_DIST = assets/hamburger.bmp \
	assets/pen24.bmp

SOURCES = $(pdf_viewer_lvgl_SOURCES)
am_pdf_viewer_lvgl_OBJECTS = $(pdf_viewer_lvgl_SOURCES:.c=.o)

pdf_viewer_lvgl_LDADD = liblvgl.a
$(STATIC_LIB): $(liblvgl_a_OBJECTS)
	$(AR) rcs $(STATIC_LIB) $(liblvgl_a_OBJECTS)

assets_%_bmp.c: assets/%.bmp
	xxd -i $< > $@
