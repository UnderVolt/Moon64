/**
 * Create an ADPCM codebook either by extracting it from an AIFF section, or
 * by executing tabledesign.
 */
#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "tabledesign/tabledesign.h"

typedef short s16;
typedef int s32;
typedef unsigned char u8;
typedef unsigned int u32;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define BSWAP16(x)
# define BSWAP32(x)
#else
# define BSWAP16(x) x = __builtin_bswap16(x)
# define BSWAP32(x) x = __builtin_bswap32(x)
#endif

#define NORETURN __attribute__((noreturn))
#define UNUSED __attribute__((unused))

typedef struct
{
    u32 start;
    u32 end;
    u32 count;
    s16 state[16];
} ALADPCMloop;

static const char usage[] = "input.aiff";

#define checked_fread(a, b, c, d) if (fread(a, b, c, d) != c) aiff_fail_parse("error parsing file")

NORETURN
void aiff_fail_parse(const char *fmt, ...)
{
    char *formatted = NULL;
    va_list ap;
    va_start(ap, fmt);
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (size >= 0) {
        size++;
        formatted = malloc(size);
        if (formatted != NULL) {
            va_start(ap, fmt);
            size = vsnprintf(formatted, size, fmt, ap);
            va_end(ap);
            if (size < 0) {
                free(formatted);
                formatted = NULL;
            }
        }
    }

    if (formatted != NULL) {
        free(formatted);
    }
    exit(1);
}

s32 read_aifc_codebook(FILE *fhandle, s32 ****table, s16 *order, s16 *npredictors)
{
    checked_fread(order, sizeof(s16), 1, fhandle);
    BSWAP16(*order);
    checked_fread(npredictors, sizeof(s16), 1, fhandle);
    BSWAP16(*npredictors);
    *table = malloc(*npredictors * sizeof(s32 **));
    for (s32 i = 0; i < *npredictors; i++) {
        (*table)[i] = malloc(8 * sizeof(s32 *));
        for (s32 j = 0; j < 8; j++) {
            (*table)[i][j] = malloc((*order + 8) * sizeof(s32));
        }
    }

    for (s32 i = 0; i < *npredictors; i++) {
        s32 **table_entry = (*table)[i];
        for (s32 j = 0; j < *order; j++) {
            for (s32 k = 0; k < 8; k++) {
                s16 ts;
                checked_fread(&ts, sizeof(s16), 1, fhandle);
                BSWAP16(ts);
                table_entry[k][j] = ts;
            }
        }

        for (s32 k = 1; k < 8; k++) {
            table_entry[k][*order] = table_entry[k - 1][*order - 1];
        }

        table_entry[0][*order] = 1 << 11;

        for (s32 k = 1; k < 8; k++) {
            s32 j = 0;
            for (; j < k; j++) {
                table_entry[j][k + *order] = 0;
            }

            for (; j < 8; j++) {
                table_entry[j][k + *order] = table_entry[j - k][*order];
            }
        }
    }
    return 0;
}

char* aiff_extract_codebook(char* filename) {
    s16 order = -1;
    s16 npredictors = -1;
    s32 ***coefTable = NULL;
    FILE *ifile;

    int   osize = 0;
    char* output = malloc(osize);

    if ((ifile = fopen(filename, "rb")) == NULL) {
        aiff_fail_parse("AIFF file could not be opened");
        exit(1);
    }

    char buf[5] = {0};
    checked_fread(buf, 4, 1, ifile);
    if (strcmp(buf, "FORM") != 0) aiff_fail_parse("not an AIFF file");
    checked_fread(buf, 4, 1, ifile);
    checked_fread(buf, 4, 1, ifile);
    if (strcmp(buf, "AIFF") != 0 && strcmp(buf, "AIFC") != 0) {
        aiff_fail_parse("not an AIFF file");
    }

    for (;;) {
        s32 size;
        if (!fread(buf, 4, 1, ifile) || !fread(&size, 4, 1, ifile)) break;
        BSWAP32(size);
        s32 nextOffset = ftell(ifile) + ((size + 1) & ~1);

        if (strcmp(buf, "APPL") == 0) {
            checked_fread(buf, 4, 1, ifile);
            if (strcmp(buf, "stoc") == 0) {
                u8 len;
                checked_fread(&len, 1, 1, ifile);
                if (len == 11) {
                    char chunkName[12];
                    s16 version;
                    checked_fread(chunkName, 11, 1, ifile);
                    chunkName[11] = '\0';
                    if (strcmp(chunkName, "VADPCMCODES") == 0) {
                        checked_fread(&version, sizeof(s16), 1, ifile);
                        BSWAP16(version);
                        if (version == 1) {
                            read_aifc_codebook(ifile, &coefTable, &order, &npredictors);
                        }
                    }
                }
            }
        }

        fseek(ifile, nextOffset, SEEK_SET);
    }
    fclose(ifile);

    if (coefTable == NULL) {
        char* args[] = {
            "tabledesign",
            "-s",
            "1",
            filename,
            NULL
        };
        tabledesign(sizeof(args) / sizeof(char*), args);
    } else {
        char tbuf[100];
        int tns = 100;
        snprintf(tbuf, sizeof(tbuf), "%d\n%d\n", order, npredictors);
        if(osize < osize + tns){
            output = realloc(output, osize = osize + tns);
            strncpy(output, tbuf, tns);
        }
        for (s32 i = 0; i < npredictors; i++) {
            for (s32 j = 0; j < order; j++) {
                for (s32 k = 0; k < 8; k++) {
                    snprintf(tbuf, sizeof(tbuf), "% 5d ", coefTable[i][k][j]);
                    if(osize < osize + tns){
                        output = realloc(output, osize = osize + tns);
                        strncat(output, tbuf, tns);
                    }
                }
                osize++;
                output = realloc(output, osize);
                strncat(output, "\n", 1);
            }
        }
    }
    return output;
}
