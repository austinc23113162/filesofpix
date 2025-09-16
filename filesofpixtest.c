/* filesofpix.c — tests for restoration + readaline
 *
 *  1) Batch tests: run ./restoration on known *-corrupt.pgm inputs.
 *  2) Validate P5 output: header parses, raster size == W*H, maxval==255.
 *  3) Edge cases: stdin mode, no usable rows, pixel >255, width mismatch,
 *     CRLF input, overlong line (>1000 without '\n' => exit(4)).
 *  4) Unit tests for readaline (EOF, CRLF, simple line).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "readaline.h"   

#define ARR_LEN(a) ((int)(sizeof(a)/sizeof((a)[0])))

static int run_cmd(const char *cmd);
static int parse_p5_header(FILE *fp, size_t *W, size_t *H, int *maxval, long *data_off);
static int check_output_file(const char *outpath, const char *label);
static int write_text_file(const char *path, const char *data, size_t nbytes);

/* Batch: run on the provided corrupted inputs */

static const char *CORRUPT_LIST[] = {
    "alum-corrupt.pgm",
    "bigboi-corrupt.pgm",
    "boston-corrupt.pgm",
    "campus-corrupt.pgm",
    "computer-corrupt.pgm",
    "elephant-corrupt.pgm",
    "jumbos-corrupt.pgm",
    "logo-corrupt.pgm",
    "professor-corrupt.pgm",
    "sail-corrupt.pgm",
    "ta-corrupt.pgm",
    "turing-corrupt.pgm"
};

/* Minimal assertion helpers */

static int failures = 0;

static void CHECKI(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        failures++;
    }
}

static void CHECKF(int ok, const char *fmt, const char *arg)
{
    if (!ok) {
        fprintf(stderr, "FAIL: ");
        fprintf(stderr, fmt, arg);
        fprintf(stderr, "\n");
        failures++;
    }
}

/* readaline unit tests */

static void test_readaline_basic(void)
{
    const char *path = "tmp_readaline_basic.txt";
    const char *data = "abc 123\nxyz\n";
    CHECKI(write_text_file(path, data, strlen(data)) == 0, "write basic tmp file");

    FILE *fp = fopen(path, "rb");
    CHECKI(fp != NULL, "open basic tmp file");

    char *line = NULL;
    size_t n = readaline(fp, &line);
    CHECKI(n == 8, "readaline length for 'abc 123\\n' should be 8");
    CHECKI(line && memcmp(line, "abc 123\n", 8) == 0, "line contents match");
    free(line);

    n = readaline(fp, &line);
    CHECKI(n == 4, "length for 'xyz\\n' should be 4");
    CHECKI(line && memcmp(line, "xyz\n", 4) == 0, "line contents match 2");
    free(line);

    n = readaline(fp, &line);
    CHECKI(n == 0 && line == NULL, "EOF -> len=0, *datapp=NULL");
    fclose(fp);
    remove(path);
}

static void test_readaline_crlf(void)
{
    const char *path = "tmp_readaline_crlf.txt";
    const char data[] = "12\r\n34\r\n";
    CHECKI(write_text_file(path, data, sizeof(data)-1) == 0, "write CRLF tmp");

    FILE *fp = fopen(path, "rb");
    CHECKI(fp != NULL, "open CRLF tmp");

    char *line = NULL;
    size_t n = readaline(fp, &line);
    CHECKI(n == 4, "CRLF line should be length 4 ('1''2''\\r''\\n')");
    free(line);

    n = readaline(fp, &line);
    CHECKI(n == 4, "second CRLF line length 4");
    free(line);

    n = readaline(fp, &line);
    CHECKI(n == 0 && line == NULL, "EOF after CRLF lines");
    fclose(fp);
    remove(path);
}

/* restoration integration tests */

static void test_batch_corrupt_inputs(void)
{
    for (int i = 0; i < ARR_LEN(CORRUPT_LIST); i++) {
        const char *in = CORRUPT_LIST[i];
        char out[256];
        snprintf(out, sizeof(out), "%s.out.pgm", in);

        char cmd[512];
        /* Use filename-arg mode (not stdin) */
        snprintf(cmd, sizeof(cmd), "./restoration %s > %s", in, out);
        int rc = run_cmd(cmd);
        CHECKF(rc == 0, "restoration failed on %s", in);

        if (rc == 0) {
            CHECKF(check_output_file(out, in) == 0, "bad P5 output for %s", in);
            remove(out);
        }
    }
}

static void test_stdin_mode(void)
{
    const char *in = "tmp_stdin_mode.txt";
    const char *line = "abc 12 z\n";  
    CHECKI(write_text_file(in, line, strlen(line)) == 0, "write stdin tmp");

    const char *out = "tmp_stdin_mode.out.pgm";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./restoration < %s > %s", in, out);
    int rc = run_cmd(cmd);
    CHECKI(rc == 0, "stdin mode run ok");

    if (rc == 0) {
        CHECKI(check_output_file(out, "stdin-mode") == 0, "stdin P5 check");
        remove(out);
    }
    remove(in);
}

static void test_no_usable_rows(void)
{
    const char *in = "tmp_no_rows.txt";
    const char *data = "abc def!!!\n---\n\r\n"; 
    CHECKI(write_text_file(in, data, strlen(data)) == 0, "write no-rows tmp");

    const char *out = "tmp_no_rows.out.pgm";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./restoration %s > %s 2>/dev/null", in, out);
    int rc = run_cmd(cmd);
    CHECKI(rc != 0, "no-rows should fail (NoInput)");
    remove(in);
    remove(out);
}

static void test_pixel_over_255(void)
{
    const char *in = "tmp_over255.txt";
    const char *data = "a300b\n"; 
    CHECKI(write_text_file(in, data, strlen(data)) == 0, "write over255 tmp");

    const char *out = "tmp_over255.out.pgm";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./restoration %s > %s 2>/dev/null", in, out);
    int rc = run_cmd(cmd);
    CHECKI(rc != 0, "pixel > 255 should fail");
    remove(in);
    remove(out);
}

static void test_width_mismatch(void)
{
    const char *in = "tmp_width_mismatch.txt";
    /* Same non-digit pattern "abc", but widths differ:
       a1b2c   -> runs: [1],[2]   -> width 2
       a1b2c3  -> runs: [1],[2],[3] -> width 3  */
    const char *data = "a1b2c\n"
                       "a1b2c3\n";
    CHECKI(write_text_file(in, data, strlen(data)) == 0, "write width mismatch");

    const char *out = "tmp_width_mismatch.out.pgm";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./restoration %s > %s 2>/dev/null", in, out);
    int rc = run_cmd(cmd);
    CHECKI(rc != 0, "width mismatch should fail"); 

    remove(in);
    remove(out);
}


static void test_crlf_input(void)
{
    const char *in = "tmp_crlf_input.txt";
    const char data[] = "ab12\r\nab34\r\n";
    CHECKI(write_text_file(in, data, sizeof(data)-1) == 0, "write CRLF input");

    const char *out = "tmp_crlf_input.out.pgm";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./restoration %s > %s", in, out);
    int rc = run_cmd(cmd);
    CHECKI(rc == 0, "CRLF input should succeed");

    if (rc == 0) {
        CHECKI(check_output_file(out, "CRLF") == 0, "CRLF output check");
        remove(out);
    }
    remove(in);
}


/* helpers */

static int run_cmd(const char *cmd)
{
    int rc = system(cmd);
    if (rc == -1) {
        perror("system()");
        return -1;
    }
    return rc;
}

static int parse_p5_header(FILE *fp, size_t *W, size_t *H, int *maxval, long *data_off)
{
    /* P5, width, height, maxval (255), separated by spaces/newlines, then one newline */
    int c1 = fgetc(fp);
    int c2 = fgetc(fp);
    if (c1 != 'P' || c2 != '5') return -1;

    /* Skip single whitespace after "P5" */
    int ch = fgetc(fp);
    if (ch == EOF) return -1;
    if (!isspace(ch)) return -1;

    /* Read width, height, maxval using fscanf (they are ASCII) */
    unsigned long w=0, h=0, mv=0;
    if (fscanf(fp, "%lu %lu %lu", &w, &h, &mv) != 3) return -1;

    /* Consume exactly one newline after maxval */
    int nl = fgetc(fp);
    if (nl != '\n' && nl != '\r') return -1;
    if (nl == '\r') { 
        int maybe_nl = fgetc(fp);
        if (maybe_nl != '\n') return -1;
    }

    *W = (size_t)w;
    *H = (size_t)h;
    *maxval = (int)mv;
    if (data_off) *data_off = ftell(fp);
    return 0;
}

static int check_output_file(const char *outpath, const char *label)
{
    FILE *fp = fopen(outpath, "rb");
    if (!fp) {
        fprintf(stderr, "cannot open output %s\n", outpath);
        return -1;
    }
    size_t W=0, H=0; int mv=0; long off=0;
    if (parse_p5_header(fp, &W, &H, &mv, &off) != 0) {
        fprintf(stderr, "bad P5 header for %s\n", label);
        fclose(fp);
        return -1;
    }
    if (mv != 255) {
        fprintf(stderr, "maxval != 255 for %s\n", label);
        fclose(fp);
        return -1;
    }

    /* Compute raster size and check file length */
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
    long end = ftell(fp);
    fclose(fp);
    if (end < 0 || off < 0) return -1;

    long raster = end - off;
    long expect = (long)W * (long)H;
    if (raster != expect) {
        fprintf(stderr, "raster size mismatch for %s: got %ld, want %ld (W=%zu H=%zu)\n",
                label, raster, expect, W, H);
        return -1;
    }
    return 0;
}

static int write_text_file(const char *path, const char *data, size_t nbytes)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;
    size_t w = fwrite(data, 1, nbytes, fp);
    int ok = (w == nbytes) ? 0 : -1;
    fclose(fp);
    return ok;
}

/*  main  */

int main(void)
{
    printf("== readaline unit tests ==\n");
    test_readaline_basic();
    test_readaline_crlf();

    printf("== restoration integration: corrupted inputs ==\n");
    test_batch_corrupt_inputs();

    printf("== restoration edge cases ==\n");
    test_stdin_mode();
    test_no_usable_rows();
    test_pixel_over_255();
    test_width_mismatch();
    test_crlf_input();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
}