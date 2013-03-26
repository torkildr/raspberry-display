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
    /* Stop at first newline */
    char *newline = strchr(input, '\n');
    if (newline != NULL)
        newline[0] = '\0';

    char *split = strchr(input, ':');
    if (split == NULL) {
        command[0] = '\0';
        return input;
    }

    /* Split command and text */
    int index = split - input;
    strncpy(command, input, index);
    command[index] = '\0';

    return input + index + 1;
}

void handle_input(char *input)
{
    char command[MAX_BUF];
    char *text = extract_command(input, command);

    printf("command: %s\n", command);
    printf("text: %s\n", text);

    if (!strcmp("time", command)) {
        display_time(text);
        display_scroll(SCROLL_RESET);
    } else if (!strcmp("scroll-left", command)) {
        display_scroll(SCROLL_LEFT);
    } else if (!strcmp("scroll-right", command)) {
        display_scroll(SCROLL_RIGHT);
    } else if (!strcmp("scroll-none", command)) {
        display_scroll(SCROLL_DISABLED);
    } else if (!strcmp("scroll-reset", command)) {
        display_scroll(SCROLL_RESET);
    } else if (!strcmp("scroll-auto", command)) {
        display_scroll(SCROLL_AUTO);
    } else if (!strcmp("text-time", command)) {
        display_text(text, 1);
    } else if (!strcmp("text", command)) {
        display_text(text, 0);
    }
}

int main()
{
    int fd;
    char input[MAX_BUF];
    char *display_fifo = "/tmp/matrix_display";

    display_enable();
    timer_enable();

    display_text("Waiting for input...", 0);

    mkfifo(display_fifo, 0666);
    fd = open(display_fifo, O_RDONLY);

    int len;
    for (;;) {
        len = read(fd, input, MAX_BUF);

        if (len <= 0) {
            if (len == -1) {
                /* Call was interrupted, don't worry */
                if (errno == EINTR)
                    continue;

                /* Ignore these */
                if (errno != EBADF)
                    printf("Read error: %s\n", strerror(errno));
            }

            close(fd);
            fd = open(display_fifo, O_RDONLY);
        } else {
            if (len >= MAX_BUF)
                input[MAX_BUF - 1] = '\0';
            else
                input[len] = '\0';

            handle_input(input);
        }
    }

    timer_disable();
    display_disable();

    close(fd);
    unlink(display_fifo);

    return 0;
}

