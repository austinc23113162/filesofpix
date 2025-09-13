#include <stdlib.h>

#include "readaline.h"

size_t readaline(FILE *inputfd, char **datapp)
{
    int size = 1000;
    *datapp = malloc(size);
    fgets(*datapp, 1000, inputfd);
    
    return size;
}