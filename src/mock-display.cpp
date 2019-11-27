#include <chrono>
#include <cstring>
#include <sstream>

#include <curses.h>

#include "display_impl.hpp"

namespace display
{

int _brightness;
std::chrono::time_point<std::chrono::steady_clock> _last_update;

float getElapsed()
{
    std::chrono::time_point now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now - _last_update).count();

    _last_update = now;
    return ms / 1e6;
}

void showInfoText(std::stringstream stream)
{
    stream << std::endl;
    addstr(stream.str().c_str());
}

void showElapsedTime(float elapsed)
{
    std::stringstream info;
    info.precision(2);

    info << "Update frequency: " << elapsed << " sec";

    showInfoText(std::move(info));
}

void showTextInfo(size_t size)
{
    std::stringstream info;

    info << "Rendered text size: " << size;

    showInfoText(std::move(info));
}

void showBrightness(int brightness)
{
    std::stringstream info;

    info << "Brightness: " << (brightness & 0xF);

    showInfoText(std::move(info));
}

void showScrolling(display::Scrolling dir, int offset)
{
    std::stringstream info;
    std::string direction;
    switch (dir)
    {
    case Scrolling::DISABLED:
        direction = "Disabled";
        break;
    case Scrolling::ENABLED:
        direction = "Enabled";
        break;
    case Scrolling::RESET:
        direction = "Reset";
        break;
    }

    info << "Scrolling: " << direction << ", Offset: " << offset;
    showInfoText(std::move(info));
}

DisplayImpl::DisplayImpl(std::function<void()> preUpdate, std::function<void()> postUpdate)
    : Display(preUpdate, postUpdate)
{
    initscr();
    timeout(0);
    cbreak();
    noecho();
    halfdelay(5);

    nonl();
    intrflush(stdscr, false);
    keypad(stdscr, true);

    getElapsed();
}

DisplayImpl::~DisplayImpl()
{
    endwin();
}

void DisplayImpl::setBrightness(int brightness)
{
    _brightness = brightness;
}

void DisplayImpl::update()
{
    clear();
    float elapsed = getElapsed();

    int row, i;
    for (row = -1; row <= 8; row++)
    {
        if (row == -1)
        {
            addch(ACS_ULCORNER);
            for (i = 0; i < X_MAX + 2; i++)
                addch(ACS_HLINE);
            addch(ACS_URCORNER);
            addch('\n');
            continue;
        }

        if (row == 8)
        {
            addch(ACS_LLCORNER);
            for (i = 0; i < X_MAX + 2; i++)
                addch(ACS_HLINE);
            addch(ACS_LRCORNER);
            addch('\n');
            continue;
            continue;
        }

        addch(ACS_VLINE);
        addch(' ');

        int col;
        for (col = 0; col < X_MAX; col++)
        {
            if (displayBuffer[col] & (1 << row))
                addch(ACS_CKBOARD);
            else
                addch(' ');
        }

        addch(' ');
        addch(ACS_VLINE);
        addch('\n');
    }

    showElapsedTime(elapsed);
    showBrightness(_brightness);
    showTextInfo(renderedTextSize);
    showScrolling(scrollDirection, scrollOffset);

    ::refresh();
}

} // namespace display
