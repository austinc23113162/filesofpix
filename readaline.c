#include <stdlib.h>
#include <string.h>

#include "readaline.h"

size_t readaline(FILE *inputfd, char **datapp)
{
    int size = 1000;
    *datapp = malloc(size);
    if (*datapp == NULL) {
        fprintf(stderr, "Failed to allocate memory");
        return 0;
    }

    if(fgets(*datapp, size, inputfd) == NULL) {
        free(*datapp);
        *datapp = NULL;
        return 0;
    };
    
    return strlen(*datapp);
}