/* restoration.c */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "except.h"
#include "mem.h"
#include "seq.h"
#include "table.h"
#include "readaline.h"
#include "atom.h"

/* Exception variables */
static const Except_T ArgsBad = { "restoration: bad arguments" };
static const Except_T OpenFail = { "restoration: could not open file" };
static const Except_T NoInput  = { "restoration: no useable rows" };
static const Except_T WidthBad = { "restoration: inconsistent row widths" };
static const Except_T PixelBad = { "restoration: pixel out of range (0-255)" };
static const Except_T WriteFail= { "restoration: write error" };  

/* This struct represents one line of the file and its width */
typedef struct Bucket {
    size_t width;
    Seq_T rows;
} *Bucket;

static void run(FILE *in);

static const char *make_pattern_key(const char *line, size_t n);

static size_t compact_digits_to_bytes(char *buf, size_t n);

static void free_bucket_cb(const void *k, void **v, void *cl);

static void obtain_sequence(FILE *in, Table_T buckets, const char **best_key,
                            size_t *best_count);

static void store_sequence(Table_T buckets, const char *key, char *row_buf,
                           size_t row_width, const char **best_key, 
                           size_t *best_count);


/********** main ********
 *
 * Parameters:
 *      int argc:     number of arguments given in the command-line
 *      char *argv[]: array that stores all the arguments
 *
 * Return: 
 *      true if all scores are under limit, false if not
 *
 * Expects:
 *      a filename given in the command-line or stdin
 * Notes:
 *      CRE if more than one argument given, input file cannot be opened, error 
 *      encountered reading file, memory allocation fails
 ************************/
int main(int argc, char *argv[])
{
    if (argc > 2) {
        RAISE(ArgsBad);
    }

    FILE *in = NULL;

    if (argc == 2) {
        in = fopen(argv[1], "rb");
        if (in == NULL) {
            RAISE(OpenFail);
        }
    } else {
        /* read from standard input per spec */
        in = stdin;
    }

    /* Run the restoration program */
    run(in);
    
    return 0;
}

/********** run ********
 *
 * Runs the restoration program.
 * 
 * Parameters:
 *      FILE *in: a pointer to a file object (hacked PGM file)
 *
 * Return: 
 *      none
 *
 * Expects:
 *      a hacked PGM file to be restored
 * 
 ************************/
static void run(FILE *in)
{
    /* Create a Hanson table */
    Table_T buckets = Table_new(0, NULL, NULL);

    /*
     * Variables to keep track of the key (Atom) in the Table that corresponds 
     * to the original lines (for later use) 
     */
    const char *best_key = NULL;
    size_t best_count = 0;

    /* Go through each line of the file and obtain/store relevant info */
    obtain_sequence(in, buckets, &best_key, &best_count);

    if (in != stdin) {
        fclose(in);
    }
    if (best_key == NULL) {
        RAISE(NoInput);
    }

    /* Go to the value of the Table that stores the restored lines */
    Bucket win = Table_get(buckets, best_key);
    size_t W = win->width;
    size_t H = Seq_length(win->rows);

    /* Print the header of the PGM 5 image */
    if (printf("P5\n%zu %zu\n255\n", W, H) < 0) {
        RAISE(WriteFail);
    }

    /* Walk through each element of the sequence (each line of the pgm )*/
    for (size_t r = 0; r < H; r++) {
        char *row = Seq_get(win->rows, (int)r);
        size_t wrote = fwrite(row, 1, W, stdout);
        if (wrote != W) {
            RAISE(WriteFail);
        }
    }

    /* Free memory */
    Table_map(buckets, free_bucket_cb, NULL);
    Table_free(&buckets);
}

/********** obtain_sequence ********
 *
 * Parses non-digit sequence and store restored digit bytes into Table from 
 *   each line.
 * 
 * Parameters:
 *      FILE *in:              pointer to the file to be read
 *      Table_T buckets:       the Table that will store all the lines
 *      const char **best_key: address of the pointer to the key in the Table 
 *                               that will store the restored lines
 *      size_t *best_count:    address of the variable that will store the                             
 *                               number of lines in the restored file
 *
 * Return: none
 *
 ************************/
static void obtain_sequence(FILE *in, Table_T buckets,
                            const char **best_key, size_t *best_count)
{
    while (1) {
        char *line = NULL;
        size_t n = readaline(in, &line);    
        if (n == 0) break;

        const char *key = make_pattern_key(line, n);

        size_t row_w = 0;
        int skip = 0;

        /* Parse digits -> bytes in place; skip row if any pixel > 255 */
        TRY {
            row_w = compact_digits_to_bytes(line, n);  
        } EXCEPT(PixelBad) {
            skip = 1;                                  
        } END_TRY;

        if (skip || row_w == 0) {                    
            FREE(line);
            continue;
        }

        RESIZE(line, row_w);                          
        store_sequence(buckets, key, line, row_w, best_key, best_count);
        /* ownership of 'line' transferred into the bucket */
    }
}

/********** make_pattern_key ********
 *
 * Parses the non-digit sequence out of the line and store it in an Atom
 * 
 * Parameters:
 *      const char *line: a line from the file
 *      size_t n:         the length of the line
 *
 * Return: 
 *      return the Atom storing the non-digit sequence which will act as a key 
 *      to the Table
 *
 ************************/
static const char *make_pattern_key(const char *line, size_t n) 
{
    /* String to store the nondigit bytes */
    char *tmp = ALLOC(n + 1);
    /* Variable to keep track of the number of nondigit bytes */
    size_t out = 0;

    /* Walk through the whole line */
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)line[i];
        if (c == '\n') {
            break;
        }
        if (!isdigit(c)) {
            /* Append the nondigit byte */
            tmp[out++] = (char)c;
        }
    }
    tmp[out] = '\0';
    /* Store the nondigit sequence in an atom and return it*/
    const char *key = Atom_string(tmp);
    FREE(tmp);
    return key;
}

/********** compact_digits_to_bytes ********
 *
 * Walk through the line can compact the digits into bytes and store it back in 
 * the original buffer
 * 
 * Parameters:
 *      char *buf: the line to compact
 *      size_t n:  the length of the line
 *
 * Return: 
 *      the size of the compact line
 *
 ************************/
static size_t compact_digits_to_bytes(char *buf, size_t n)
{
    size_t i = 0, out = 0;
    /* Walk through the line */
    while (i < n) {
        unsigned char c = (unsigned char)buf[i];
        if (c == '\n') {
            break;
        }
        if (isdigit(c)) {
            unsigned v = 0;
            /* Parse consecutive digit bytes into v */
            do {
                /* Append consecutive digits */
                v = v * 10u + (unsigned)(buf[i] - '0');
                i++;
            } while (i < n && isdigit((unsigned char)buf[i]));
            if (v > 255u) {
                RAISE(PixelBad);
            }
            /* Store the digit bytes back into the original buffer */
            buf[out++] = (char)(unsigned char)v;
        } else {
            i++;
        }
    }
    return out;
}

/********** free_bucket_cb ********
 *
 * Frees the strings in the Sequence and the Sequence itself
 * 
 * Parameters:
 *      const void *k: pointer to the key of the table
 *      void **v:      address of the pointer to the value of the table
 *      void *cl:      pointer to the closure variable
 *
 * Return: 
 *      none
 *
 ************************/
static void free_bucket_cb(const void *k, void **v, void *cl) 
{
    (void)k; (void)cl;
    Bucket b = *(Bucket *)v;
    if (!b) return;

    size_t h = Seq_length(b->rows);
    for (size_t r = 0; r < h; r++) {
        char *row = Seq_get(b->rows, (int)r);
        FREE(row);
    }
    Seq_free(&b->rows);
    FREE(b);
}

/********** store_sequence ********
 *
 * Using the non-digit Atom as the key, store its corresponding restored lines 
 * in a Sequence in the Table
 * 
 * Parameters:
 *      Table_T buckets:       the Table to store restored lines
 *      const char *key:       the Atom key used to store lines in the table
 *      char *row_buf:
 *      size_t row_width:      the width of each line
 *      const char **best_key: address of the pointer to the key in the Table 
 *                             that will store the restored lines
 *      size_t *best_count:    address of the variable that will store the 
 *                             number of lines in the restored file
 *
 * Return: 
 *      true if all scores are under limit, false if not
 *
 ************************/
static void store_sequence(Table_T buckets, const char *key, char *row_buf,
                           size_t row_width, const char **best_key, 
                           size_t *best_count) 
{
    Bucket b = Table_get(buckets, key);
    /* If the nondigit sequence key has not been stored yet */
    if (b == NULL) {
        /* Allocate memory for the Bucket struct and initialize it */
        b = ALLOC(sizeof(*b));
        b->width = row_width;
        b->rows = Seq_new(0);
        /* Insert into table */
        Table_put(buckets, key, b);
    } else if (row_width != b->width) {
        RAISE(WidthBad);
    }

    /* Append the parsed line to the sequence */
    Seq_addhi(b->rows, row_buf);
    size_t cnt = Seq_length(b->rows);

    /* The seq with the longest length is the one that has the original lines */
    if (cnt > *best_count) {
        *best_count = cnt;
        *best_key = key;
    }
}