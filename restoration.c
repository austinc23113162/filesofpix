#include <stdlib.h>
#include "readaline.h"

int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Please only provide the file path\n");
    }
    else if (argc == 2) {
        FILE *fp = fopen(argv[1], "rb");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s %s %s\n", argv[0], "Could not open file", argv[1], "for reading");
            return EXIT_FAILURE;
        }

        char *string;
        while (readaline(fp, &string)) {
            char *temp = string;
            while (*temp != '\0') {
                printf("%c ", (unsigned char)*temp);
                temp++;
            }
            free(string);
            string = NULL;
        }
        
        free(string);  

        fclose(fp);
    }
    else {
        printf("Read from standard input\n");
    }
    return EXIT_SUCCESS;
}

