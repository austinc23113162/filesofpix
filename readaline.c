#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "readaline.h"
#include "except.h"
#include "mem.h"

static const Except_T Readaline_BadArgs = { "readaline: bad arguments" };
static const Except_T Readaline_ReadErr = { "readaline: read error" };

size_t readaline(FILE *inputfd, char **datapp)
{
    //Hanson excpetion handling for bad arguments
    if (inputfd == NULL || datapp == NULL) {
        RAISE(Readaline_BadArgs);               
    }

    //Allocates memory for the buffer
    const size_t CAP = 1000;
    char *buf = ALLOC(CAP + 1);

    //Reads a line from the file into the buffer
    if (fgets(buf, (int)CAP + 1, inputfd) == NULL) {
        int had_error = ferror(inputfd);
        FREE(buf);
        if (had_error) {
            RAISE(Readaline_ReadErr);
        }
        *datapp = NULL;
        return 0;
    }

    size_t len = strlen(buf);

    //Checks if the line is too long and the last character is not a newline
    if (len == CAP && buf[len - 1] != '\n') {
        fputs("readline: input line too long\n", stderr);
        exit(4);
    }

    *datapp = buf;
    
    //Length of the string including the null terminator
    return len;
}
