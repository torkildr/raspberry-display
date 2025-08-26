#include "display.hpp"

namespace display
{

class DisplayImpl : public Display
{
public:
    DisplayImpl(
        std::function<void()> preUpdate,
        std::function<void()> postUpdate,
        DisplayStateCallback stateCallback,
        std::function<void()> scrollCompleteCallback
    );
    virtual ~DisplayImpl();

    void setBrightness(int brightness);

private:
    void update();
    
    // Track current brightness for restoration after reinitialization
    int currentBrightness = DEFAULT_BRIGHTNESS;
};

} // namespace display
