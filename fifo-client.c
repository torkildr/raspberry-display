#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "display.h"

#define MAX_BUF 1024

char *time_string()
{
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);
    return asctime(local_time);
}

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

int offset = 0;

void handle_input(char *input)
{
    char command[MAX_BUF];
    char *text = extract_command(input, command);

    if (!strcmp("time", command))
        text = time_string();
    else if (!strcmp("scroll-left", command))
        display_scroll(SCROLL_LEFT);
    else if (!strcmp("scroll-right", command))
        display_scroll(SCROLL_RIGHT);
    else if (!strcmp("text", command))
        display_scroll(SCROLL_DISABLED);

    render_text(text, offset++);
}

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

            handle_input(input);
        }
    }

    timer_disable();
    display_disable();

    close(fd);
    unlink(display_fifo);

    return 0;
}

