#!/bin/bash

DEFINITIONS='-D__x86_64__="1" -D__STDC_HOSTED__="1" -D__STDC_EMBED_EMPTY__="2" -D__STDC_UTF_16__="1" -D__STDC_IEC_559__="1" -D__STDC_ISO_10646__="201706L" -D__STDC_IEC_559_COMPLEX__="1" -D_STDC_PREDEF_H="1" -D__STDC_IEC_60559_COMPLEX__="201404L" -D__STDC_IEC_60559_BFP__="201404L" -D__STDC_EMBED_FOUND__="1" -D__STDC_EMBED_NOT_FOUND__="0" -D__STDC_VERSION__="202311L" -D__GNUC_STDC_INLINE__="1" -D__STDC_UTF_32__="1" -D__STDC__="1" -D__unix__="1" -Dlinux="1" -D__amd64__="1" -D__LP64__="1"'



INCLUDE="-I/usr/include/c++/15 -I/usr/include/c++/15/x86_64-redhat-linux -I/usr/include/c++/15/backward -I/usr/lib/gcc/x86_64-redhat-linux/15/include -I/usr/local/include -I/usr/include "

INCLUDE_prj="-I. -Isrc -I./lvgl -I./lvgl/src"

INCLUDE_LIB="-I/usr/include/librsvg-2.0 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/webp -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/libxml2 -I/usr/include/freetype2 -I/usr/include/libpng16 -DWITH_GZFILEOP -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/sysprof-6 -I/usr/include/pixman-1 -I/usr/local/include -I/usr/local/include/cjson"

CFLAGS="${DEFINITIONS} ${INCLUDE} ${INCLUDE_prj} ${INCLUDE_LIB}"
#CFLAGS="${DEFINITIONS} ${INCLUDE_prj} "

OPTIONS="-isrc/test.c --suppressions-list=suppress.txt --checkers-report=cpp_check_report.txt"


script -q -c "cppcheck --check-level=exhaustive --enable=all --inconclusive --std=c++17 src/ ${CFLAGS} ${OPTIONS} " cppcheck.log

aha < cppcheck.log > cppcheck.html
