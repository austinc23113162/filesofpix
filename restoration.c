#include <stdlib.h>
#include "readaline.h"
#include "mem.h"
#include "string.h"
#include "atom.h"

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
        char *nondig_seq = ALLOC(1);
        nondig_seq[0] = '\0';
        int nondig_len = 0;   
        while (*temp != '\0') { /* Walk through each character*/
            /* Is a digit */
            if (*temp > 47 && *temp < 58) {
                //printf("%c", *temp);
            }
            /* Is not a digit */
            else {
                nondig_len++;
                RESIZE(nondig_seq, nondig_len);
                nondig_seq[nondig_len - 1] = *temp;
                nondig_seq[nondig_len] = '\0';
            }
            temp++;
        }
        printf("%s", nondig_seq);
        FREE(string);
        FREE(nondig_seq);
    }

    
    
}
