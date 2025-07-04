#ifndef TA_HOVER_POD_H
#define TA_HOVER_POD_H

#include "object_set.h"

class TA_HoverPod : public TA_Object {
private:
    bool direction, idle = true;
    int rangeLeft, rangeRight;

    const float speed = 0.33;

public:
    using TA_Object::TA_Object;
    void load(TA_Point newPosition, int range, bool flip);
    bool update() override;
};

#endif // TA_HOVER_POD_H
