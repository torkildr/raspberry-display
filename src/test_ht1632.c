#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "display.h"
#include "ht1632.h"

#define FIRST_N 50

void print_buffer(unsigned char *buffer, int length)
{
    for(int n=0; n < length; ++n) {
        int pos = n / 8;
        int bit = n % 8;

        printf("%d", (buffer[pos] >> (7 - bit) & 1) ? 1 : 0);
    }
    printf("\n");
}

void foo() {
    uint16_t data = HT1632_ID_WRITE;
    data <<= HT1632_LENGTH_DATA;
    data |= 0b11000111;
    data <<= (16 - HT1632_LENGTH_DATA - HT1632_LENGTH_ID);
    reverse_endian(&data, sizeof(data));

    print_buffer((unsigned char *) &data, 16);
    printf("\n");
}

int main()
{
    foo();

    memset(display_memory, 0, sizeof(display_memory));
    memset(ht1632_write_buffer, 0, sizeof(ht1632_write_buffer));
    sprintf((char *) display_memory, " ,.ABa;");

    print_buffer(display_memory, FIRST_N);

    update_write_buffer(0);

    print_buffer(ht1632_write_buffer, FIRST_N);

    return 0;
}

