#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <smf.h>
#include <stdbool.h>
#include <sys/queue.h>
#include "v_common.h"
#include "v_misc.h"
#include "v_midi.h"
#include "v_musicxml.h"

#define MIDI_EVENT_NOTE_ON 0x90
#define MIDI_EVENT_NOTE_OFF 0x80
#define MIDI_EVENT_CC 0xC0
#define MUSICXML_EXT_LEN 9

static v_status_t v_midi_init(void);
static v_status_t v_midi_open(const char *path);
static int v_midi_pagenum(void);
static v_status_t v_midi_size(int *width, int *height);
static v_status_t v_midi_pixel(uint8_t *data, int page, int rowstride, v_scale_t ctm);
static void v_midi_free(void);
static v_annots_t *v_midi_annots(void);
static void v_midi_save(const char *path);

static v_draw_ops_t g_ops;
static v_draw_ops_t *mxl_ops;
static v_midi_t *g_midi;
static char mxl_file[] = "/tmp/temp_svg_XXXXXX.musicxml";

const char *gm_part_name[] = {
    "A.Grand Piano",
    "Bright A.Piano",
    "E.Grand Piano",
    "Honky-Tonk Piano",
    "E.Piano 1",
    "E.Piano 2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tublar Bells",
    "Dulcimer",
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",
    "A.Guitar(Nylon)",
    "A.Guitar(Steel)",
    "E.Guitar(Jazz)",
    "E.Guitar(Clean)",
    "E.Guitar(Muted)",
    "E.Guitar(Overdriven)",
    "E.Guitar(Distortion)",
    "Guitar Harmonics",
    "A.Bass",
    "E.Bass(Finger)",
    "E.Bass(Pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",
    "String Ensemble 1",
    "String Ensemble 2",
    "Synth Strings 1",
    "Synth Strings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Hone",
    "Brass Section",
    "Synth Brass 1",
    "Synth Brass 2",
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Hone",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead1(Square)",
    "Lead2(Sawtooth)",
    "Lead3(Calliope)",
    "Lead4(Chiff)",
    "Lead5(Charang)",
    "Lead6(Voice)",
    "Lead7(Fifths)",
    "Lead8(Bass+Lead)",
    "Pad1(New Age)",
    "Pad2(Warm)",
    "Pad3(Polysynth)",
    "Pad4(Choir)",
    "Pad5(Bowed)",
    "Pad6(Metallic)",
    "Pad7(Halo)",
    "Pad8(Sweep)",
    "FX1(Rain)",
    "FX2(Soundtrack)",
    "FX3(Crystal)",
    "FX4(Atmosphere)",
    "FX5(Brightness)",
    "FX6(Goblins)",
    "FX7(Echoes)",
    "FX8(Sci-Fi)",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag Pipe",
    "Fiddle",
    "Shanai",
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot",
};
extern const char *MXL_HEADER;
extern const char *MXL_SCORE_PART_TEMPLATE;
extern const char *MXL_END_PART_LIST;
extern const char *MXL_PART_HEADER_TEMPLATE;
extern const char *MXL_MEASURE_START_TEMPLATE;
extern const char *MXL_NOTE_TEMPLATE;
extern const char *MXL_NOTE_ALTER_TEMPLATE;
extern const char *MXL_REST_TEMPLATE;
extern const char *MXL_MEASURE_END;
extern const char *MXL_PART_END;
extern const char *MXL_SCORE_END;

static void init_ops(void)
{
    if (!g_ops.init)
    {
        g_ops.init = v_midi_init;
        g_ops.open = v_midi_open;
        g_ops.size = v_midi_size;
        g_ops.pagenum = v_midi_pagenum;
        g_ops.pixel = v_midi_pixel;
        g_ops.annots = v_midi_annots;
        g_ops.save = v_midi_save;
        g_ops.free = v_midi_free;

        mxl_ops = v_musicxml_get_ops();
    }
}
v_draw_ops_t *v_midi_get_ops(void)
{
    init_ops();
    return &g_ops;
}

extern char *sjis_to_utf8(uint8_t *, size_t);

TAILQ_HEAD(tq_head, str_v_note)
_head;

void print_event(smf_event_t *event)
{
    fflush(stdout);
    if (event->midi_buffer[0] != 0xff)
    {
        printf("  MIDI Event at time %u: ", event->time_pulses);
        if ((event->midi_buffer[0] & 0x50) == 0x50)
        {
        }
        else
        {
            for (unsigned int i = 0; i < event->midi_buffer_length; i++)
            {
                printf("%02X ", event->midi_buffer[i]);
            }
            printf("\n");
        }
    }
    else if (event->midi_buffer[0] == 0xff)
    {
        printf("  Meta Event at time %u: type 0x%02X\n", event->time_pulses, event->midi_buffer[1]);
        if (event->midi_buffer[1] == 0x01 || // Text Event
            event->midi_buffer[1] == 0x03 || // Track Name
            event->midi_buffer[1] == 0x04 || // Instrument Name
            event->midi_buffer[1] == 0x05 || // Lyrics
            event->midi_buffer[1] == 0x07 || // Lyrics
            event->midi_buffer[1] == 0x06)   // Marker
        {
            printf("    Text: %.*s\n", (int)event->midi_buffer_length - 3, &event->midi_buffer[3]);
        }
        else
        {
            char *s = b2s(event->midi_buffer, min(event->midi_buffer_length, 16));
            printf("    %s\n", s);
        }
    }
}

static bool smf_header_expect(const char *buf, const char *expect)
{
    return (strncmp(expect, buf, 4) == 0);
}
static bool is_xfih(const char *buf)
{
    return smf_header_expect(buf, "XFIH");
}
static bool is_xfkm(const char *buf)
{
    return smf_header_expect(buf, "XFKM");
}
static bool is_xfhd(const char *buf)
{
    return smf_header_expect(buf, "XFhd");
}
static bool is_xfln(const char *buf)
{
    return smf_header_expect(buf, "XFln");
}

uint32_t read_deltatime(const uint8_t *buf, int32_t *index)
{
    uint32_t delta_time;
    uint8_t byte;
    *index = 0;
    delta_time = 0;
    do
    {
        byte = buf[(*index)++];
        delta_time = (delta_time << 7) | (byte & 0x7F);
    } while (byte & 0x80);

    return delta_time;
}

static bool is_jp(uint8_t *buf)
{
    return buf && buf[0] == 'J' && buf[1] == 'P';
}

static void analyze_xfih_xfln(xfih_t *xfih)
{
    ERR_RETn(!xfih->xfln_base);

    char *cur;
    char *p;
    p = cur = xfih->xfln_base;
    d("%.*s", xfih->xfln_len, xfih->xfln_base);
    for (xfln_item_t i = XFLN_START; i < XFLN_MAX; i++)
    {
        while ((*cur) != ':')
            cur++;
        *cur = '\0';
        if (p == cur)
        {
            d("not found");
            xfih->xfln[i] = NULL;
        }
        else
        {
            d("found %s", p);
            xfih->xfln[i] = p;
        }
        p = ++cur;
    }
    if (p != &xfih->xfln_base[xfih->xfln_len])
    {
        xfih->xfln[XFLN_MAX] = p;
        xfih->xfln_base[xfih->xfln_len + 1] = '\0';
    }
    else
    {
        xfih->xfln[XFLN_MAX] = NULL;
    }
    for (xfln_item_t i = XFLN_START; i < XFLN_MAX; i++)
    {
        if (xfih->xfln[i])
        {
            d("%d: %s", i, xfih->xfln[i]);
        }
        else
        {
            d("%d: not found", i);
        }
    }
error_return:
    return;
}

static void analyze_xfih_xfih(xfih_t *xfih)
{
    ERR_RETn(!xfih->xfih_base);

    char *cur;
    char *p;
    p = cur = xfih->xfih_base;
    for (xfih_item_t i = XFIH_START; i < XFIH_MAX; i++)
    {
        while ((*cur) != ':')
            cur++;
        *cur = '\0';
        if (p == cur)
        {
            xfih->xfih[i] = NULL;
        }
        else
        {
            xfih->xfih[i] = p;
        }
        p = ++cur;
    }
    if (p != &xfih->xfih_base[xfih->xfih_len])
    {
        xfih->xfih[XFIH_MAX] = p;
        xfih->xfih_base[xfih->xfih_len + 1] = '\0';
    }
    else
    {
        xfih->xfih[XFIH_MAX] = NULL;
    }
    for (xfih_item_t i = XFIH_START; i < XFIH_MAX; i++)
    {
        if (xfih->xfih[i])
        {
            d("%d: %s", i, xfih->xfih[i]);
        }
        else
        {
            d("%d: not found", i);
        }
    }
error_return:
    return;
}

static xfih_t *analyze_xfih(uint8_t *buf, uint32_t len)
{
    xfih_t *ret = NULL;
    xfih_t *xfih;
    int32_t index = 0;
    uint8_t buflen;
    uint8_t *p = buf;
    uint32_t delta_time;

    xfih = malloc(sizeof(xfih_t));
    while ((p - buf) < len)
    {
        delta_time = read_deltatime(p, &index);
        (void)delta_time;
        p += index;
        ERR_RET(p[0] != 0xff, "not meta data");

        buflen = read_deltatime(&p[2], &index);
        if (p[1] != 1)
        {
            p += index + 2;
            p += buflen;
            continue;
        }

        printf("p: %.*s\n", 4, &p[index + 2]);
        if (is_xfln((const char *)&p[index + 2]))
        {
            if (is_jp(&p[index + 7]))
            {
                xfih->xfln_base = sjis_to_utf8(&p[index + 10], buflen - 8);
                xfih->xfln_len = strlen(xfih->xfln_base);
            }
            else
            {
                // JP, L1以外は後回し
                xfih->xfln_len = buflen - 8;
                xfih->xfln_base = malloc(xfih->xfln_len + 1);
                memcpy(&p[index + 10], xfih->xfln_base, xfih->xfln_len);
            }
            analyze_xfih_xfln(xfih);
        }
        else if (is_xfhd((const char *)&p[index + 2]))
        {
            xfih->xfih_base = malloc(buflen - 4);
            xfih->xfih_len = buflen - 5;
            memcpy(xfih->xfih_base, &p[2 + index + 5], buflen - 5);
            d("%.*s", buflen - 5, xfih->xfih_base);
            analyze_xfih_xfih(xfih);
        }
        p += index + 2;
        p += buflen;
    }

    ret = xfih;
error_return:
    return ret;
}

static bool is_xflh(uint8_t *buf, uint32_t len)
{
    return len >= 5 && buf && memcmp(buf, "$Lyrc", 5) == 0;
}

static void analyze_xfkm_xflh(xfkm_t *xfkm, char *buf, uint32_t len)
{
    ERR_RETn(!buf);

    char *cur;
    char *p;
    p = cur = buf;

    for (xfkm_xflh_t i = XFLH_START; i < XFLH_MAX; i++)
    {
        while ((*cur) != ':')
            cur++;
        *cur = '\0';
        d("xflh(%d): %s", i, p);
        switch (i)
        {
        case XFLH_ID:
            break;
        case XFLH_LANG:
            d("LANG ERROR");
            break;
        case XFLH_MELO_PART:
            char *b = p;
            int c = 0;
            for (char *a = p; a < cur; a++)
            {
                if ((*a) == ',')
                {
                    *a = '\0';
                    xfkm->melo_parts[c++] = min(0, atoi(b));
                    b = a + 1;
                }
            }
            break;
        case XFLH_OFFSET:
            xfkm->disp_offset = min(0, atoi(p));
            break;
        default:
            break;
        }
        p = ++cur;
    }
    if (p != &buf[len])
    {
        d("%.*s", 2, p);
        memcpy(&xfkm->lang, p, 2);
    }
    else
    {
        xfkm->lang = 0;
    }
error_return:
    return;
}

static xfkm_t *analyze_xfkm(uint8_t *buf, uint32_t len)
{
    d("%p (%x)", buf, len);
    xfkm_t *xfkm;
    xfkm_t *ret = NULL;
    uint32_t event_num;
    uint32_t esize;
    int32_t dlen;
    uint32_t delta;

    // 最初に全探査してイベントの数を調べる
    esize = 0;
    event_num = 0;
    for (uint8_t *p = buf; (p - buf) < len; p += esize)
    {
        delta = read_deltatime(p, &dlen);
        p += dlen;
        ERR_RET((*p) != 0xff, "not meta info found");

        esize = read_deltatime(&p[2], &dlen);
        p += dlen + 2;
        event_num++;
    }
    ERR_RETn(!event_num);

    d("n: %d", event_num);

    xfkm = calloc(sizeof(xfkm_t) + sizeof(xfih_event_t) * event_num, 1);
    xfkm->event_num = event_num;
    xfkm->events = (xfih_event_t *)&xfkm[1];

    for (uint8_t *p = buf, i = 0; (p - buf) < len; p += esize, i++)
    {
        delta = read_deltatime(p, &dlen);
        p += dlen;
        esize = read_deltatime(&p[2], &dlen);
        xfkm->events[i].bytes = NULL;
        xfkm->events[i].delta_time = delta;
        xfkm->events[i].num = 0;
        switch (p[1])
        {
        case 7:
            if (is_xflh(&p[dlen + 2], esize))
            {
                analyze_xfkm_xflh(xfkm, (char *)&p[dlen + 2], esize);
            }
            // XF Lyrics Header 以外は無視
            break;
        case 5:
            if (is_jp((uint8_t *)&xfkm->lang))
            {
                xfkm->events[i].bytes = (uint8_t *)sjis_to_utf8(&p[dlen + 2], esize);
                xfkm->events[i].num = strlen((char *)xfkm->events[i].bytes);
                d("%s", xfkm->events[i].bytes);
            }
            else
            {
                xfkm->events[i].bytes = malloc(esize + 1);
                memcpy(xfkm->events[i].bytes, &p[dlen + 2], esize);
                xfkm->events[i].bytes[esize] = '\0';
                xfkm->events[i].num = strlen((char *)xfkm->events[i].bytes);
            }
            break;
        default:
            break;
        }
        p += dlen + 2;
        //            char *s = b2s(&p[dlen + 2], esize);
        //            d("%02X %02X d:%x %s", p[0], p[1], esize, s);
    }

    ret = xfkm;
error_return:
    return ret;
}

static yamaha_xf_t *load_xf(const char *name)
{
    uint8_t buf[8];
    uint32_t len;
    int status;
    yamaha_xf_t *ret = NULL;
    yamaha_xf_t *xf;
    FILE *fp = fopen(name, "rb");
    ERR_RET(!fp, "fopen failed");

    xf = calloc(sizeof(yamaha_xf_t), 1);
    ERR_RET(!xf, "malloc");
    while ((status = fread(buf, 8, 1, fp)) == 1)
    {
        len = __builtin_bswap32(*(uint32_t *)&buf[4]);
        printf("%.*s %x\n", 4, buf, len);
        if (is_xfih((char *)buf))
        {
            uint8_t *d = malloc(len);
            fread(d, len, 1, fp);
            xf->xfih = analyze_xfih(d, len);
        }
        else if (is_xfkm((char *)buf))
        {
            uint8_t *d = malloc(len);
            fread(d, len, 1, fp);
            xf->xfkm = analyze_xfkm(d, len);
        }
        else
        {
            fseek(fp, len, SEEK_CUR);
            d("next: %lx", ftell(fp));
        }
    }

    fclose(fp);

    ret = xf;
error_return:
    return ret;
}

static v_midi_t *load_midi(const char *filename)
{
    v_midi_t *ret = NULL;
    v_midi_t *midi;
    struct tq_head *head;
    char *s;
    uint16_t note_num[16] = {0};
    smf_t *smf = smf_load(filename);
    ERR_RET(!smf, "Failed to load MIDI file: %s\n", filename);

    midi = calloc(sizeof(v_midi_t), 1);
    midi->smf = smf;
    midi->xf = load_xf(filename);

    printf("Loaded MIDI file: %s\n", filename);
    printf("Format: %d\n", smf->format);
    printf("Number of tracks: %d\n", smf->number_of_tracks);
    printf("Division (ticks per quarter note): %d\n\n", smf->resolution);

    for (int i = 1; i < 16; i++)
    {
        smf_track_t *track = smf_get_track_by_number(smf, i);
        if (!track)
            continue;

        smf_event_t *last = smf_track_get_last_event(track);
        for (int j = 1;; j++)
        {
            smf_event_t *event = smf_track_get_event_by_number(track, j);
            if (smf_event_is_metadata(event))
            {
                size_t len;
                switch (event->midi_buffer[1])
                {
                case 3:
                    len = event->midi_buffer_length - 3;
                    midi->title = malloc(len + 1);
                    memcpy(midi->title, &event->midi_buffer[3], len);
                    midi->title[len] = '\0';
                    break;
                case 0x51:
                    ERR_RET(event->midi_buffer_length < 6, "Tempo defunct");
                    // テンポチェンジはここでは取らない
                    if (midi->tempo)
                        continue;

                    s = b2s(event->midi_buffer, event->midi_buffer_length);
                    d("%06d: 51 found %s", j, s);
                    midi->tempo = 0;
                    memcpy(&((uint8_t *)&midi->tempo)[1], &event->midi_buffer[3], 3);

                    d("bef tempo %x", midi->tempo);
                    midi->tempo = __builtin_bswap32(midi->tempo);
                    midi->tempo = (60 * 1000 * 1000) / midi->tempo;
                    break;
                case 0x58:
                    ERR_RET(event->midi_buffer_length < 7, "Tempo defunct");
                    midi->numerator = event->midi_buffer[3];
                    midi->denominator = npow(2, event->midi_buffer[4]);
                    midi->measure = last->time_pulses / (4 * smf->ppqn * midi->numerator / midi->denominator);
                    printf("ttl measure:%d", midi->measure);
                    break;
                default:
                    break;
                }
            }
            if (smf_event_is_sysex(event))
                continue;

            //            print_event(event);
            if ((event->midi_buffer[0] & 0xf0) == MIDI_EVENT_NOTE_ON)
            {
                uint8_t channel = event->midi_buffer[0] & 0xf;
                if (event->midi_buffer[2])
                    note_num[channel]++;
            }

            if (event == last)
            {
                d("event num:%d", j);
                break;
            }
        }
    }

    s = malloc(64);
    char *p = s;
    for (int i = 0; i < 16; i++)
    {
        p += sprintf(p, "%d,", note_num[i]);
        midi->channel[i].note_num = note_num[i];
        midi->channel[i].name = NULL;
        printf("ch:%d note:%d\n", i, note_num[i]);
        if (note_num[i])
        {
            midi->channel[i].notes = malloc(sizeof(v_note_t) * note_num[i]);
        }
        note_num[i] = 0;
    }

    d("%s %d/%d tempo: %d %s", midi->title, midi->numerator, midi->denominator, midi->tempo, s);
    free(s);

    head = &_head;
    TAILQ_INIT(head);

    for (int i = 1; i < 16; i++)
    {
        smf_track_t *track = smf_get_track_by_number(smf, i);
        if (!track)
            continue;
        smf_event_t *last = smf_track_get_last_event(track);
        smf_event_t *event = NULL;
        for (int j = 1; event != last; j++)
        {
            event = smf_track_get_event_by_number(track, j);
            bool note_off = false;
            if ((event->midi_buffer[0] & 0xf0) == MIDI_EVENT_NOTE_ON)
            {
                uint8_t channel = event->midi_buffer[0] & 0xf;
                if (event->midi_buffer[2])
                {
                    v_note_t *note = &midi->channel[channel].notes[note_num[channel]];
                    note->channel = channel;
                    note->delta_time = event->time_pulses;
                    note->pitch = event->midi_buffer[1];
                    note->velocity = event->midi_buffer[2];
                    note->value = 0;
                    note_num[channel]++;

                    TAILQ_INSERT_TAIL(head, note, entry);
                }
                else
                {
                    note_off = true;
                }
            }
            else if ((event->midi_buffer[0] & 0xf0) == MIDI_EVENT_NOTE_OFF)
            {
                note_off = true;
            }
            if (note_off)
            {
                uint8_t channel = event->midi_buffer[0] & 0xf;
                uint8_t pitch = event->midi_buffer[1];
                v_note_t *note;
                bool found = false;
                TAILQ_FOREACH(note, head, entry)
                {
                    if (note->channel == channel && note->pitch == pitch)
                    {
                        TAILQ_REMOVE(head, note, entry);
                        note->value = event->time_pulses - note->delta_time;
                        // d("pos:%d pitch:%x len:%d", note->delta_time, note->pitch, note->value);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    d("note off without note on");
                }
            }
            if ((event->midi_buffer[0] & 0xf0) == MIDI_EVENT_CC)
            {
                uint8_t channel = event->midi_buffer[0] & 0xf;
                if (!midi->channel[channel].name && !(event->midi_buffer[1] & 0x80))
                {
                    midi->channel[channel].name = gm_part_name[event->midi_buffer[1]];
                }
                // printf("%d: d:%d %02x %02x\n", j, event->time_pulses, event->midi_buffer[0], event->midi_buffer[1]);
            }
        }
    }

    //    smf_delete(smf);
    ret = midi;
error_return:
    return ret;
}

static int get_measure_rest(v_midi_t *midi)
{
    return 4 * midi->smf->ppqn * midi->numerator / midi->denominator;
}

static char *midi_to_musicxml(v_midi_t *midi)
{
    char *ret = NULL;
    struct
    {
        char step;
        int8_t alter;
    } pitch_char[] = {
        {'C', 0},
        {'C', 1},
        {'D', 0},
        {'D', 1},
        {'E', 0},
        {'F', 0},
        {'F', 1},
        {'G', 0},
        {'G', 1},
        {'A', 0},
        {'A', 1},
        {'B', 0},
    };
    char buf[512];

    ERR_RET(!midi, "no input");

    int fd = mkstemps(mxl_file, MUSICXML_EXT_LEN);
    ERR_RET(fd < 0, "mkstemps");
    close(fd);

    FILE *fp = fopen(mxl_file, "w");
    ERR_RET(!fp, "fopen");
    d("mxl to %s", mxl_file);
    printf("mxl to %s\n", mxl_file);

    fprintf(fp, "%s", MXL_HEADER);

    int rest_base = get_measure_rest(midi);
    for (int i = 0; i < 16; i++)
    {
        if (!midi->channel[i].note_num)
            continue;

        snprintf(buf, sizeof(buf), MXL_SCORE_PART_TEMPLATE, i + 1, midi->channel->name ?: (char *)&i);
        fprintf(fp, "%s", buf);
    }
    fprintf(fp, "%s", MXL_END_PART_LIST);

    for (int i = 0; i < 16; i++)
    {
        printf("ch:%d processing...\n", i + 1);

        v_note_t *note;
        for (int j = 0, measure = 0; measure < midi->measure; measure++)
        {
            if (!measure)
            {
                snprintf(buf, sizeof(buf), MXL_PART_HEADER_TEMPLATE, i + 1, measure + 1, midi->numerator, midi->denominator);
                fprintf(fp, "%s", buf);
            }
            else
            {
                snprintf(buf, sizeof(buf), MXL_MEASURE_START_TEMPLATE, measure + 1);
                fprintf(fp, "%s", buf);
            }
            note = &midi->channel[i].notes[j++];
            if (j >= midi->channel[i].note_num || !note || !note->pitch)
            {
                snprintf(buf, sizeof(buf), MXL_REST_TEMPLATE, rest_base, "whole");
                fprintf(fp, "%s", buf);
                fprintf(fp, "%s", MXL_MEASURE_END);

                continue;
            }
            while (j < midi->channel[i].note_num && !note->value)
            {
                note = &midi->channel[i].notes[j++];
            }
            if (note->delta_time >= (measure + 1) * rest_base)
            {
                // 全休符
                snprintf(buf, sizeof(buf), MXL_REST_TEMPLATE, rest_base, "whole");
                fprintf(fp, "%s", buf);
                fprintf(fp, "%s", MXL_MEASURE_END);
                continue;
            }

            int rest;
            if (note->delta_time % rest_base)
            {
                // 小節頭の休符
                rest = rest_base - (note->delta_time % rest_base);

                // TODO: あとで考える
                snprintf(buf, sizeof(buf), MXL_REST_TEMPLATE, rest, "eighth");
                fprintf(fp, "%s", buf);
                fflush(fp);
            }

            rest = rest_base;
            while (note->delta_time < (measure + 1) * rest_base)
            {
                char step = pitch_char[note->pitch % 12].step;
                int8_t alter = pitch_char[note->pitch % 12].alter;
                int8_t octave = (note->pitch / 12) - 1;
                if (alter)
                {
                    // TODO: あとで
                    snprintf(buf, sizeof(buf), MXL_NOTE_ALTER_TEMPLATE, step, alter, octave, note->value, "eighth");
                    fprintf(fp, "%s", buf);
                    fflush(fp);
                    rest -= note->value;
                }
                else
                {
                    // TODO: あとで考える
                    snprintf(buf, sizeof(buf), MXL_NOTE_TEMPLATE, step, octave, note->value, "eighth");
                    fprintf(fp, "%s", buf);
                    fflush(fp);
                    rest -= note->value;
                }
                note = &midi->channel[i].notes[j++];
                while (j < midi->channel[i].note_num && note && !note->value)
                {
                    note = &midi->channel[i].notes[j++];
                }
            }
            if (rest > 0)
            {
                // TODO: あとで考える
                snprintf(buf, sizeof(buf), MXL_REST_TEMPLATE, rest, "eighth");
                fprintf(fp, "%s", buf);
                fflush(fp);
            }
            fprintf(fp, "%s", MXL_MEASURE_END);
            fflush(fp);
        }
        fprintf(fp, "%s", MXL_PART_END);
        fflush(fp);
    }
    fprintf(fp, "%s", MXL_END_PART_LIST);
    fflush(fp);

    fclose(fp);

    ret = mxl_file;
error_return:
    return ret;
}
static v_status_t v_midi_init(void)
{
    return mxl_ops->init();
}

static v_status_t v_midi_open(const char *path)
{
    g_midi = load_midi(path);

    char *mpath = midi_to_musicxml(g_midi);
    return mxl_ops->open(mpath);
}
static int v_midi_pagenum(void)
{
    return mxl_ops->pagenum();
}
static v_status_t v_midi_size(int *width, int *height)
{
    return mxl_ops->size(width, height);
}
static v_status_t v_midi_pixel(uint8_t *data, int page, int rowstride, v_scale_t ctm)
{
    return mxl_ops->pixel(data, page, rowstride, ctm);
}
static void v_midi_free(void)
{
    return mxl_ops->free();
}
static v_annots_t *v_midi_annots(void)
{
    return mxl_ops->annots();
}
static void v_midi_save(const char *path)
{
    return mxl_ops->save(path);
}

const char *MXL_HEADER = R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE score-partwise PUBLIC
    "-//Recordare//DTD MusicXML 3.1 Partwise//EN"
    "http://www.musicxml.org/dtds/partwise.dtd">
<score-partwise version="3.1">
  <part-list>
)";
const char *MXL_SCORE_PART_TEMPLATE = R"(<score-part id="P%d">
      <part-name>%s</part-name>
    </score-part>
)";
const char *MXL_END_PART_LIST = "  </part-list>\n";

const char *MXL_PART_HEADER_TEMPLATE = R"(  <part id="P%d">
    <measure number="%d">
      <attributes>
        <divisions>480</divisions>
        <key>
          <fifths>0</fifths>
        </key>
        <time>
          <beats>%d</beats>
          <beat-type>%d</beat-type>
        </time>
        <clef>
          <sign>G</sign>
          <line>2</line>
        </clef>
      </attributes>
)";
const char *MXL_MEASURE_START_TEMPLATE = R"(    <measure number="%d">
)";
const char *MXL_NOTE_TEMPLATE = R"(      <note>
        <pitch>
          <step>%c</step>
          <octave>%d</octave>
        </pitch>
        <duration>%d</duration>
        <type>%s</type>
      </note>
)";
const char *MXL_NOTE_ALTER_TEMPLATE = R"(      <note>
        <pitch>
          <step>%c</step>
          <alter>%d</alter>
          <octave>%d</octave>
        </pitch>
        <duration>%d</duration>
        <type>%s</type>
      </note>
)";
const char *MXL_REST_TEMPLATE = R"(      <note>
          <rest/>
          <duration>%d</duration>
          <type>%s</type>
      </note>
)";

const char *MXL_MEASURE_END = "    </measure>\n";
const char *MXL_PART_END = "  </part>\n";
const char *MXL_SCORE_END = "</score-partwise>\n";
