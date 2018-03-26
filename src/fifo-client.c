#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "display.h"
#include "fifo-client.h"

#define MAX_BUF 1024

void exit_clean()
{
    timer_disable();
    display_disable();

    printf("Exiting\n");
    exit(EXIT_SUCCESS);
}

void exit_handler(int sig)
{
    printf("Caught exit signal: %d\n", sig);
    exit_clean();
}

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

    if (!strcmp("time", command)) {
        display_time(text);
        display_scroll(SCROLL_RESET);
    } else if (!strcmp("scroll", command)) {
        if (!strcmp("left", text)) {
            display_scroll(SCROLL_LEFT);
        } else if (!strcmp("right", text)) {
            display_scroll(SCROLL_RIGHT);
        } else if (!strcmp("none", text)) {
            display_scroll(SCROLL_DISABLED);
        } else if (!strcmp("reset", text)) {
            display_scroll(SCROLL_RESET);
        } else if (!strcmp("auto", text)) {
            display_scroll(SCROLL_AUTO);
        }
    } else if (!strcmp("brightness", command)) {
        int brightness = atoi(text);
        if (brightness >= 0 && brightness <= 0xF) {
          display_brightness(brightness);
        }
    } else if (!strcmp("text-time", command)) {
        display_text(text, 1);
    } else if (!strcmp("text", command)) {
        display_text(text, 0);
    } else if (!strcmp("quit", command)) {
        exit_clean();
    }
}

int main(int argc, char *argv[])
{
    static char *display_fifo;

    signal(SIGINT, exit_handler);
    signal(SIGTERM, exit_handler);

    if (argc >= 2) {
        display_fifo = argv[1];
    } else {
        display_fifo = "/tmp/raspberry-display";
    }

    printf("Reading from pipe: %s\n", display_fifo);

    int fd;

    unlink(display_fifo);
    mkfifo(display_fifo, 0666);
    if ((fd = open(display_fifo, O_RDONLY | O_NONBLOCK)) == -1) {
        perror("Error opening pipe");
        exit(EXIT_FAILURE);
    }

    display_enable();
    timer_enable();

    display_text("Waiting for input...", 0);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    
    char read_buffer[MAX_BUF];

    for(;;) {
        int retval;
        TEMP_FAILURE_RETRY(retval = select(fd + 1, &read_fds, NULL, NULL, NULL));

        if (retval == -1) {
            perror("Could not start reading from pipe");
            exit(EXIT_FAILURE);
        } if (retval) {
            int len;
            TEMP_FAILURE_RETRY(len = read(fd, read_buffer, MAX_BUF - 1));

            if (len == -1) {
                perror("Error during read from pipe");
                exit(EXIT_FAILURE);
            }

            read_buffer[len] = '\0';

            const char *token = strtok(read_buffer, "\n");
            while (token != NULL) {
                char *input = strdup(token);
                handle_input(input);
                free(input);                

                token = strtok(NULL, "\n");
            }
        }
    }

    timer_disable();
    display_disable();

    close(fd);
    unlink(display_fifo);

    printf("Exiting\n");

    return 0;
}

