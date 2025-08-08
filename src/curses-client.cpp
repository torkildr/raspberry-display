#include <curses.h>

#include "display_impl.hpp"

static void init_curses()
{
    initscr();
    timeout(0);
    cbreak();
    noecho();
    halfdelay(10);

    nonl();
    intrflush(stdscr, false);
    keypad(stdscr, true);
}

static void print_help_text()
{
    addstr("\nt: time");
    addstr("\na: supported characters");
    addstr("\nb: supported characters + time");
    addstr("\n0: reset offset");
    addstr("\nl: enable scrolling");
    addstr("\nr: reset scrolling");
    addstr("\n+/-: change brightness");
    addstr("\nd: disable scroll");
    addstr("\nc: toggle center/left alignment");
    addstr("\nq: exit");
    refresh();
}

std::string abc_string = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\xe6\xf8\xe5\xc6\xd8\xc5";

int main()
{
    auto preUpdate = [] {
        clear();
    };
    auto postUpdate = [] {
        print_help_text();
    };

    init_curses();

    auto disp = display::DisplayImpl(preUpdate, postUpdate);
    disp.start();
    disp.showTime("%H:%M:%S", "Foobar, binbaz... Tralala, ding dong!");

    bool done = false;
    int brightness = 7;
    display::Alignment currentAlignment = display::Alignment::LEFT;
    disp.setBrightness(brightness);

    while (!done)
    {
        int c;

        if ((c = getch()) != ERR)
        {
            switch (c)
            {
            case 'q':
                done = true;
                break;
            case 't':
                disp.showTime("");
                break;
            case 'a':
                disp.show(abc_string);
                break;
            case 'b':
                disp.showTime("", abc_string);
                break;
            case 'l':
                disp.setScrolling(display::Scrolling::ENABLED);
                break;
            case 'r':
                disp.setScrolling(display::Scrolling::RESET);
                break;
            case '+':
                if (brightness < 0xF)
                {
                    disp.setBrightness(++brightness);
                }
                break;
            case '-':
                if (brightness > 1)
                {
                    disp.setBrightness(--brightness);
                }
                break;
            case 'd':
            case '0':
            case KEY_HOME:
                disp.setScrolling(display::Scrolling::DISABLED);
                break;
            case 'c':
                // Toggle alignment between LEFT and CENTER
                if (currentAlignment == display::Alignment::LEFT)
                {
                    currentAlignment = display::Alignment::CENTER;
                }
                else
                {
                    currentAlignment = display::Alignment::LEFT;
                }
                disp.setAlignment(currentAlignment);
                break;
            }
        }
    }

    disp.stop();
    endwin();

    return EXIT_SUCCESS;
}
