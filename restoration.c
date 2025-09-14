#include <stdlib.h>
#include "readaline.h"

void obtain_sequence();

int main(int argc, char *argv[])
{
    /* Checked runtime error */
    if (argc > 2) {
        printf("Please only provide the file path\n");
    }
    else if (argc == 2) {
        FILE *fp = fopen(argv[1], "rb");
        /* Checked runtime error */
        if (fp == NULL) {
            fprintf(stderr, "%s: %s %s %s\n", argv[0], "Could not open file", argv[1], "for reading");
            return EXIT_FAILURE;
        }

        obtain_sequence(fp);
        

        fclose(fp);
    }
    else {
        printf("Read from standard input\n");
        char filepath[1024];
        if (fgets(filepath, sizeof(filepath), stdin) != NULL) {
            printf("yay file given\n");
        }
    }
    return EXIT_SUCCESS;
}

void obtain_sequence(FILE *fp)
{
    char *string;
    /* For each line of the file */
    while (readaline(fp, &string)) {
        /* Temp string to loop through so original can be freed at the end*/
        char *temp = string;
        /* Walk through each character*/
        while (*temp != '\0') {
            if (*temp > 47 && *temp < 58) {
                printf("%d", *temp);
            }
            else {
                printf("not digit: %d", *temp);
            }
            temp++;
        }
        printf("\n");
        free(string);
        string = NULL;
    }

    
    
}
