#include <stdlib.h>
#include "readaline.h"
#include "mem.h"

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("Please provide a file\n");
    }
    else {
        FILE *fp = fopen(argv[1], "rb");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s %s %s\n", argv[0], "Could not open file", argv[1], "for reading");
            return EXIT_FAILURE;
        }

        char *string;
        while (readaline(fp, &string)) {
            char *temp = string;
            while (*temp != '\0') {
                if (*temp > 31) {
                    printf("%c ", *temp);
                }
                temp++;
            }
            FREE(string);
            string = NULL;
        }
        
        fclose(fp);
    }
    return EXIT_SUCCESS;
}