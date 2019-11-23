#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <thread>
#include <chrono>

#include "display_impl.hpp"

using namespace boost::beast::websocket;

int main()
{
    auto disp = display::DisplayImpl([]{}, []{});

    disp.start();
    disp.setBrightness(8);

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    disp.stop();
}
