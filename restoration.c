#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "except.h"
#include "mem.h"
#include "seq.h"
#include "table.h"
#include "readaline.h"
#include "mem.h"
#include "string.h"
#include "atom.h"

/* Exception variables */
static const Except_T ArgsBad = { "restoration: bad arguments" };
static const Except_T OpenFail = { "restoration: could not open file" };
static const Except_T NoInput = { "restoration: no useable rows" };
static const Except_T WidthBad = { "restoration: inconsistent row widths" };
static const Except_T PixelBad = { "restoration: pixel out of range (0-255)" };
static const Except_T WriteFail = { "restoration: write error" };  

/* This struct represents one line of the file and its width */
typedef struct Bucket {
    size_t width;
    Seq_T rows;
} *Bucket;


static const char *make_pattern_key(const char *line, size_t n);

static size_t compact_digits_to_bytes(char *buf, size_t n);

static void free_bucket_cb(const void *k, void **v, void *cl);

static void obtain_sequence(FILE *in, Table_T buckets, const char **best_key,
                            size_t *best_count);

static void store_sequence(Table_T buckets, const char *key, char *row_buf,
                           size_t row_width, const char **best_key, 
                           size_t *best_count);



int main(int argc, char *argv[])
{
    if (argc > 2) {
        RAISE(ArgsBad);
    }

    FILE *in = NULL;
    char filename[1024];

    /* Filename is given in command-line */
    if(argc == 2) {
        in = fopen(argv[1], "rb");
        if (in == NULL) {
            RAISE(OpenFail);
        }
    }
    else {
        /* Get filename from stdin */
        if (fgets(filename, sizeof(filename), stdin) == NULL) {
            RAISE(OpenFail);
        }

        filename[strcspn(filename, "\n")] = '\0';
        in = fopen(filename, "rb");
        if (in == NULL) {
            RAISE(OpenFail);
        }
    }

    Table_T buckets = Table_new(0, NULL, NULL);
    /*
    /  Variables to keep track of the key (Atom) in the Table that corresponds  
    /  to the original lines (for later use) 
    */
    const char *best_key = NULL;
    size_t best_count = 0;

    obtain_sequence(in, buckets, &best_key, &best_count);

    if (in != stdin) {
        fclose(in);
    }
    if (best_key == NULL) {
        RAISE(NoInput);
    }
    Bucket win = Table_get(buckets, best_key);
    size_t W = win->width;
    size_t H = Seq_length(win->rows);
    if (printf("P5\n%zu %zu\n255\n", W, H) < 0) {
        RAISE(WriteFail);
    }
    for (size_t r = 0; r < H; r++) {
        char *row = Seq_get(win->rows, (int)r);
        size_t wrote = fwrite(row, 1, W, stdout);
        if (wrote != W) {
            RAISE(WriteFail);
        }
    }
    Table_map(buckets, free_bucket_cb, NULL);
    Table_free(&buckets);
    return 0;
}

void obtain_sequence(FILE *in, Table_T buckets, const char **best_key,
                            size_t *best_count)
{
    /* Series of actions for each line of the file */
    while(1){
        char *line = NULL;
        /* readaline returns the number of bytes read */
        size_t n = readaline(in, &line);
        if (n == 0) {
            break;
        }

        /* Parses and returns non-digit sequence from the line */
        const char *key = make_pattern_key(line, n);
        /* Parses the digit sequence of the line into bytes and replace line with that sequence */
        size_t row_w = compact_digits_to_bytes(line, n);

        /* If there are no digits in the line */
        if (row_w == 0) {
            FREE(line);
            continue;
        }

        /* Resize line to fit the parsed sequence */
        RESIZE(line, row_w);

        /* Store the parsed line into a Table */
        store_sequence(buckets, key, line, row_w, best_key, best_count);
    }
}


static const char *make_pattern_key(const char *line, size_t n) 
{
    char *tmp = ALLOC(n + 1);
    /* Variable to keep track of the number of nondigit bytes */
    size_t out = 0;

    /* Walk through the whole line */
    for (size_t i = 0; i < n; i++) {
        /* Cast to a byte */
        // unsigned char c = (unsigned char)line[i];
        char c = line[i];
        if (c == '\n') {
            break;
        }
        if (!isdigit(c)) {
            /* Append the nondigit byte */
            tmp[out++] = (char)c;
        }
    }
        tmp[out] = '\0';
        const char *key = Atom_string(tmp);
        FREE(tmp);
        return key;
}

static size_t compact_digits_to_bytes(char * buf, size_t n)
{
    size_t i = 0, out = 0;
    while (i < n) {
        unsigned char c = (unsigned char)buf[i];
        if (c == '\n') {
            break;
        }
        if (isdigit(c)) {
            unsigned v = 0;
            do {
                v = v * 10u + (unsigned)(buf[i] - '0');
                i++;
            } while (i < n && isdigit((unsigned char)buf[i]));
            if (v > 255u) {
                RAISE(PixelBad);
            }
            buf[out++] = (char)(unsigned char)v;
        } else {
            i++;
        }
    }
    return out;
}

static void free_bucket_cb(const void *k, void **v, void *cl) 
{
   (void)k; (void)cl;
    Bucket b = *(Bucket *)v;
    if (!b) {
        return;
    }
    size_t h = Seq_length(b->rows);
    for (size_t r = 0; r < h; r++) {
        char *row = Seq_get(b->rows, (int)r);
        FREE(row);
    }
    Seq_free(&b->rows);
    FREE(b);
}

static void store_sequence(Table_T buckets, const char *key, char *row_buf,
                           size_t row_width, const char **best_key, 
                           size_t *best_count) 
{
    Bucket b = Table_get(buckets, key);
    if (b == NULL) {
        b = ALLOC(sizeof(*b));
        b->width = row_width;
        b->rows = Seq_new(0);
        Table_put(buckets, key, b);
    } else if (row_width != b->width) {
        RAISE(WidthBad);
    }
    Seq_addhi(b->rows, row_buf);
    size_t cnt = Seq_length(b->rows);
    if (cnt > *best_count) {
        *best_count = cnt;
        *best_key = key;
    }
}