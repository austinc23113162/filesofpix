#include <stdio.h>
#include <stdlib.h>

#include "readaline.h"
#include "except.h"
#include "mem.h"

static const Except_T Readaline_BadArgs = {"readaline: bad arguments"};
static const Except_T Readaline_ReadErr = {"readaline: read error"};

/********** readaline ********
 *
 * Reads one line of bytes from inputfd into a newly allocated buffer.
 *
 * Parameters:
 *      FILE *inputfd : input stream
 *      char **datapp : out-parameter for allocated line buffer
 *
 * Returns: The number of bytes read and stored in the buffer, or 0 if EOF.
 *
 ************************/
size_t readaline(FILE *inputfd, char **datapp)
{
        if (inputfd == NULL || datapp == NULL)
        {
                RAISE(Readaline_BadArgs);
        }

        /* dynamically grow to handle arbitrary-length lines  */
        size_t cap = 128;
        char *buf = ALLOC(cap + 1);
        size_t used = 0;
        int ch;

        /* Walk through every byte */
        while ((ch = fgetc(inputfd)) != EOF)
        {
                /* Resize the buffer when full */
                if (used == cap)
                {
                        size_t new_cap = cap * 2;
                        RESIZE(buf, new_cap + 1);
                        cap = new_cap;
                }
                buf[used++] = (char)ch;
                if (ch == '\n')
                {
                        break;
                }
        }

        if (ferror(inputfd))
        {
                FREE(buf);
                RAISE(Readaline_ReadErr);
        }

        if (used == 0 && ch == EOF)
        {
                /* EOF before any bytes */
                FREE(buf);
                *datapp = NULL;
                return 0;
        }

        buf[used] = '\0';
        *datapp = buf;
        return used;
}