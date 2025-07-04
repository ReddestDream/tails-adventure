#include "anti_air_missile.h"
#include "explosion.h"

void TA_AntiAirMissile::load(TA_Point position) {
    loadFromToml("objects/anti_air_missile.toml");
    hitbox.setRectangle({2, 2}, {14, 14});
    explosionSound.load("sound/explosion.ogg", TA_SOUND_CHANNEL_SFX3);
    this->position = position;
    collisionType = TA_COLLISION_ATTACK | TA_COLLISION_BOMB | TA_COLLISION_DRILL;
    updatePosition();

    TA_Rect aimHitbox;
    aimHitbox.setRectangle({-24, -128}, {24, 0});
    aimHitbox.setPosition(position);
    bool centerGood = (objectSet->checkCollision(aimHitbox) & TA_COLLISION_TARGET) != 0;

    aimHitbox.setPosition(position - TA_Point(48, 0));
    bool leftGood = (objectSet->checkCollision(aimHitbox) & TA_COLLISION_TARGET) != 0;

    aimHitbox.setPosition(position + TA_Point(48, 0));
    bool rightGood = (objectSet->checkCollision(aimHitbox) & TA_COLLISION_TARGET) != 0;

    if(centerGood) {
        velocity = {0, -4};
        setFrame(0);
    } else if(rightGood) {
        velocity = {3, -3};
        setFrame(1);
    } else if(leftGood) {
        velocity = {-3, -3};
        setFrame(2);
    } else {
        velocity = {0, -4};
        setFrame(0);
    }
}

bool TA_AntiAirMissile::update() {
    if(!destroyed) {
        position = position + velocity * TA::elapsedTime;
        updatePosition();
        if((position - objectSet->getCharacterPosition()).length() > 200) {
            return false;
        }
        if(objectSet->checkCollision(hitbox) & (TA_COLLISION_SOLID | TA_COLLISION_TARGET)) {
            objectSet->spawnObject<TA_Explosion>(position, 0, TA_EXPLOSION_CHARACTER);
            explosionSound.play();
            // TODO: explosion velocity
            for(int i = 1; i <= 3; i++) {
                TA_Point explosionPosition =
                    position + TA_Point(int(TA::random::next() % 7) - 3, int(TA::random::next() % 7) - 3);
                explosionSound.play();
                objectSet->spawnObject<TA_Explosion>(explosionPosition, i * 16);
            }
            hitbox.setRectangle({0, 0}, {16, 16});
            destroyed = true;
        }
    } else {
        timer += TA::elapsedTime;
        if(timer > 10) {
            return false;
        }
    }

    return true;
}
