bin_PROGRAMS = pdf_viewer_lvgl

pdf_viewer_lvgl_SOURCES = main.c \
$(shell find lvgl/src -name "*.c") \
v_menu.c \
v_pdf.c

C_INC = \
    -I$(srcdir)/lvgl \
    -I$(srcdir)/lvgl/src

AM_CFLAGS = $(C_INC) -g -O0 
AM_LDFLAGS = -lmupdf -lX11

SOURCES = $(pdf_viewer_lvgl_SOURCES)
am_pdf_viewer_lvgl_OBJECTS = $(pdf_viewer_lvgl_SOURCES:.c=.o)

