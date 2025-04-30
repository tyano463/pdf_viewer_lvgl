#ifndef __V_MIDI_H__
#define __V_MIDI_H__

#include <sys/queue.h>
#include <smf.h>
#include "v_canvas.h"
typedef struct str_xf_event
{
    uint32_t delta_time;
    uint8_t *bytes;
    uint8_t num;
} xfih_event_t;
typedef struct str_xfkm
{
    xfih_event_t *events;
    uint8_t melo_parts[4]; /**!< need 0 initialize */
    uint32_t disp_offset;
    uint16_t event_num;
    uint16_t lang;
} xfkm_t;

typedef uint8_t xfih_item_t;
typedef uint8_t xfln_item_t;
enum
{
    XFIH_DATE,
#define XFIH_START XFIH_DATE
    XFIH_COUNTRY,
    XFIH_CATEGORY,
    XFIH_BEAT,
    XFIH_MELO_PART,
    XFIH_VOCAL_TYPE,
    XFIH_COMPOSER,
    XFIH_LYRICIST,
    XFIH_ARRANGER,
    XFIH_PERFORMER,
    XFIH_PROGRAMMER,
    XFIH_KEYWORD,
#define XFIH_MAX XFIH_KEYWORD
};

enum
{
    XFLN_SONG_NAME,
#define XFLN_START XFLN_SONG_NAME
    XFLN_COMPOSER,
    XFLN_LYRICIST,
    XFLN_ARRANGER,
    XFLN_PERFORMER,
    XFLN_PROGRAMMER,
#define XFLN_MAX XFLN_PROGRAMMER
};

typedef uint8_t xfkm_xflh_t;
enum
{
    XFLH_ID,
#define XFLH_START XFLH_ID
    XFLH_MELO_PART,
    XFLH_OFFSET,
    XFLH_LANG,
#define XFLH_MAX XFLH_LANG
};

typedef struct str_xfih
{
    char *xfih[XFIH_MAX + 1];
    char *xfln[XFLN_MAX + 1];
    char *xfih_base;
    char *xfln_base;
    uint16_t xfih_len;
    uint16_t xfln_len;
} xfih_t;

typedef struct str_yamaha_xf
{
    xfih_t *xfih;
    xfkm_t *xfkm;
    uint8_t ver;
} yamaha_xf_t;

typedef struct str_v_note
{
    TAILQ_ENTRY(str_v_note)
    entry;
    uint32_t delta_time;
    uint32_t next_time;
    uint16_t value;
    uint16_t index;
    uint8_t pitch;
    uint8_t velocity;
    uint8_t channel;
} v_note_t;

typedef struct str_v_midi_channel
{
    v_note_t *notes;
    uint32_t note_num;
    char *name;
} v_midi_channel_t;

typedef struct str_v_midi
{
    char *title;
    uint32_t tempo;
    uint8_t numerator;
    uint8_t denominator;
    yamaha_xf_t *xf;
    v_midi_channel_t channel[16];
    uint32_t measure;
    smf_t *smf;
} v_midi_t;

v_draw_ops_t *v_midi_get_ops(void);

#endif