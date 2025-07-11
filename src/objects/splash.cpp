#include "splash.h"

void TA_Splash::load(TA_Point newPosition) {
    loadFromToml("objects/splash.toml");
    setAnimation("splash");

    position = newPosition;
    int top = int((position.y + 6) / 16) * 16 - 6;
    int bottom = top + 16;
    if(objectSet->getLinks().seaFox != nullptr) {
        top += 3;
        bottom += 3;
    }
    if(abs(position.y - top) < abs(position.y - bottom)) {
        position.y = top;
    } else {
        position.y = bottom;
    }

    setPosition(position);
}

bool TA_Splash::update() {
    if(!isAnimated()) {
        return false;
    }
    return true;
}
