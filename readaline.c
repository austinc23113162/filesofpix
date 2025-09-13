#include "readaline.h"

size_t readaline(FILE *inputfd, char **datapp)
{
    fgets(*datapp, 1000, inputfd);
    unsigned int x = 3;
    return x;
}