#ifndef display_impl_hpp
#define display_impl_hpp

#include "display.hpp"

namespace display {

class DisplayImpl : public Display {
    public:
        DisplayImpl(std::function<void()> preUpdate, std::function<void()> postUpdate);
        virtual ~DisplayImpl();

        void setBrightness(int brightness);

    private:
        void update();
};

}

#endif