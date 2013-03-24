#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "display.h"

#define MAX_BUF 1024

char *extract_command(char *input, char *command)
{
    char *substr = strchr(input, ':');
    if (substr == NULL)
        return input;

    int index = substr - input;
    strncpy(command, input, index);
    command[index] = '\0';

    return input + index + 1;
}

void handle_input(char *input)
{
    char command[MAX_BUF];
    char *text = extract_command(input, command);

    if (!strcmp("time", command)) {
        display_time("");
        return;
    }
    else if (!strcmp("scroll-left", command))
        display_scroll(SCROLL_LEFT);
    else if (!strcmp("scroll-right", command))
        display_scroll(SCROLL_RIGHT);
    else if (!strcmp("text", command))
        display_scroll(SCROLL_DISABLED);

    display_text(text);
}

int main()
{
    int fd;
    char input[MAX_BUF];
    char *display_fifo = "/tmp/matrix_display";

    display_enable();
    timer_enable();

    display_text("Waiting for input...");

    mkfifo(display_fifo, 0666);
    fd = open(display_fifo, O_RDONLY);

    int len;
    for (;;) {
        len = read(fd, input, MAX_BUF);

        if (len <= 0) {
            close(fd);
            fd = open(display_fifo, O_RDONLY);

            if (len < 0)
                printf("%s\n", strerror(errno));
        } else {
            if (len >= MAX_BUF)
                input[MAX_BUF - 1] = '\0';
            else
                input[len] = '\0';

            sleep(1);
            handle_input(input);
        }
    }

    timer_disable();
    display_disable();

    close(fd);
    unlink(display_fifo);

    return 0;
}

