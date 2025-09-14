#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "readaline.h"

size_t readaline(FILE *inputfd, char **datapp)
{
    if(feof(inputfd) != 0) {
        *datapp = NULL;
        return 0;
    }
    else {
        int size = 1000;
        *datapp = malloc(size);
        if (*datapp == NULL) {
            fprintf(stderr, "Failed to allocate memory");
            return 0;
        }

        fgets(*datapp, size, inputfd);
    }
    
    return strlen(*datapp);
}