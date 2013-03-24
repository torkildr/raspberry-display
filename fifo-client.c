#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "display.h"

#define MAX_BUF 1024

int main()
{
    int fd;
    char input[MAX_BUF];
    char *display_fifo = "/tmp/matrix_display";

    display_enable();
    timer_enable();

    render_text("Waiting for input...", 0);

    mkfifo(display_fifo, 0666);
    fd = open(display_fifo, O_RDONLY);

    int len;
    for (;;) {
        len = read(fd, input, MAX_BUF);
        if (len == 0) {
            close(fd);
            fd = open(display_fifo, O_RDONLY);
        } else {
            if (len >= MAX_BUF)
                input[MAX_BUF - 1] = '\0';
            else
                input[len] = '\0';

            render_text(input, 0);
        }
    }

    timer_disable();
    display_disable();

    close(fd);
    unlink(display_fifo);

    return 0;
}

