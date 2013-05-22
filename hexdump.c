#include <stdio.h>
#include "hexdump.h"

void hexdump( const char * prefix, unsigned char * b, int len )
{
    for (int i = 0; i < len; )
    {
        printf("%s%06d : ", prefix ? prefix : "", i);
        int j;
        for (j = 0; j < 16; j++)
        {
            if (j + i < len)
            {
                int n = j + i;
                unsigned char c = b[n];
                printf("%02x ", c);
            }
            else
            {
                printf("   ");
            }
            if (j == 7)
                printf(" ");
        }
        printf(": ");
        for (j = 0; j < 16 && j + i < len; j++)
        {
            int n = j + i;
            unsigned char c = b[n];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }

        printf("\n");
        i += j;
    }
}

