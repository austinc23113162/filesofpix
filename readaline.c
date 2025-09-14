#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "readaline.h"

size_t readaline(FILE *inputfd, char **datapp)
{
    if(feof(inputfd) != 0) {
        printf("At eof");
        *datapp = NULL;
        return 0;
    }
    else {
        int size = 1000;
        *datapp = malloc(size);
        /* Checked runtime error */
        if (*datapp == NULL) {
            fprintf(stderr, "Failed to allocate memory");
            return 0;
        }

        /* There should be checked runtime error here */
        if(fgets(*datapp, size, inputfd) == NULL) {
        };
    }
    
    return strlen(*datapp);
}