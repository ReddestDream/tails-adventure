#ifndef TA_CRUISER_H
#define TA_CRUISER_H

#include "object_set.h"

class TA_Cruiser : public TA_Object {
public:
    using TA_Object::TA_Object;
    void load();
    bool update() override;
    void draw() override;

private:
    enum class State { IDLE, ACTIVE, DESTROYED, POST_DESTROYED };

    void updateIdle();
    void updateActive();
    void updateDestroyed();
    void updatePostDestroyed();
    void updateBorderPosition();
    void updateBirds();
    void updateDamage();

    State state = State::IDLE;
    TA_Point lockPosition;
    bool cameraNormalized = false;

    TA_Sprite watcherSprite;
    TA_Sprite leftThrowerSprite;
    TA_Sprite rightThrowerSprite;
    int leftThrowerPrevFrame = 1;
    int rightThrowerPrevFrame = 1;

    TA_Sound hitSound;

    int health = 32;
    float speed = 1.5;

    float timer = 0;
};

#endif // TA_CRUISER_H
