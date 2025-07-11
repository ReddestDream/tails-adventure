#include "cruiser.h"
#include "barrel.h"
#include "dead_kukku.h"
#include "explosion.h"
#include "ring.h"
#include "sea_fox.h"
#include "transition.h"

void TA_Cruiser::load() {
    loadFromToml("objects/cruiser/cruiser.toml");
    hitbox.setRectangle({16, 45}, {164, 94});
    collisionType = TA_COLLISION_DAMAGE | TA_COLLISION_TARGET;
    position = {TA::screenWidth + 64, 62};

    watcherSprite.loadFromToml("objects/cruiser/watcher.toml");
    leftThrowerSprite.loadFromToml("objects/rock_thrower.toml");
    rightThrowerSprite.loadFromToml("objects/rock_thrower.toml");
    watcherSprite.setCamera(objectSet->getLinks().camera);
    watcherSprite.setAnimation("idle");
    leftThrowerSprite.setCamera(objectSet->getLinks().camera);
    leftThrowerSprite.setAnimation("throw_barrel_left");
    leftThrowerSprite.setFlip(true);
    rightThrowerSprite.setCamera(objectSet->getLinks().camera);
    rightThrowerSprite.setAnimation("throw_barrel_right");
    rightThrowerSprite.setFlip(true);

    hitboxVector.assign(2, HitboxVectorElement());
    hitboxVector[0].hitbox.setRectangle({-16, 0}, {0, TA::screenHeight});
    hitboxVector[0].collisionType = TA_COLLISION_SOLID;
    hitboxVector[1].hitbox.setRectangle({0, 0}, {16, TA::screenHeight});
    hitboxVector[1].collisionType = TA_COLLISION_SOLID;

    hitSound.load("sound/hit.ogg", TA_SOUND_CHANNEL_SFX3);

    updatePosition();
    updateBorderPosition();
    lockPosition = {32, 240 - TA::screenHeight};
    objectSet->getLinks().camera->setLockPosition(lockPosition);
}

bool TA_Cruiser::update() {
    switch(state) {
        case State::IDLE:
            updateIdle();
            break;
        case State::ACTIVE:
            updateActive();
            break;
        case State::DESTROYED:
            updateDestroyed();
            break;
        case State::POST_DESTROYED:
            updatePostDestroyed();
            break;
    }

    updatePosition();
    updateBorderPosition();
    updateBirds();
    return true;
}

void TA_Cruiser::draw() {
    watcherSprite.setUpdateAnimation(!objectSet->isPaused());
    leftThrowerSprite.setUpdateAnimation(!objectSet->isPaused());
    rightThrowerSprite.setUpdateAnimation(!objectSet->isPaused());

    TA_Object::draw();
    watcherSprite.draw();
    leftThrowerSprite.draw();
    rightThrowerSprite.draw();
}

void TA_Cruiser::updateBorderPosition() {
    hitboxVector[0].hitbox.setPosition(lockPosition);
    hitboxVector[1].hitbox.setPosition(lockPosition + TA_Point(TA::screenWidth, 0));
}

void TA_Cruiser::updateBirds() {
    watcherSprite.setPosition(position + TA_Point(72, 16));
    leftThrowerSprite.setPosition(position + TA_Point(56, 35));
    rightThrowerSprite.setPosition(position + TA_Point(152, 27));

    if(state == State::ACTIVE && cameraNormalized) {
        if(leftThrowerSprite.getCurrentFrame() == 0 && leftThrowerPrevFrame == 1) {
            objectSet->spawnObject<TA_Barrel>(
                leftThrowerSprite.getPosition() + TA_Point(12, 4), TA_Point(1 + 0.1 * (TA::random::next() % 20), -2));
        }
        if(rightThrowerSprite.getCurrentFrame() == 0 && rightThrowerPrevFrame == 1) {
            objectSet->spawnObject<TA_Barrel>(
                rightThrowerSprite.getPosition() + TA_Point(12, 4), TA_Point(1 + 0.1 * (TA::random::next() % 25), -2));
        }
        leftThrowerPrevFrame = leftThrowerSprite.getCurrentFrame();
        rightThrowerPrevFrame = rightThrowerSprite.getCurrentFrame();
    }
}

void TA_Cruiser::updateIdle() {
    if(objectSet->getLinks().camera->isLocked()) {
        TA::sound::playMusic("sound/boss.vgm");
        state = State::ACTIVE;
    }
}

void TA_Cruiser::updateActive() {
    position.x += TA::elapsedTime * 1.5F;
    float center = position.x + 104;
    if(cameraNormalized) {
        lockPosition.x += TA::elapsedTime * 1.5F;
    } else {
        lockPosition.x += TA::elapsedTime * 2;
        if(lockPosition.x > center - TA::screenWidth / 2) {
            cameraNormalized = true;
        }
    }

    objectSet->getLinks().seaFox->setVelocityAdd({1.5, 0});
    objectSet->getLinks().camera->setLockPosition(lockPosition);
    objectSet->getLinks().camera->forceLockX();

    updateDamage();
}

void TA_Cruiser::updateDamage() {
    if(!isAnimated()) {
        if(objectSet->checkCollision(hitbox) & TA_COLLISION_BOMB) {
            setAnimation("damage");
            hitSound.play();
            health--;
            if(health == 0) {
                state = State::DESTROYED;
            }
        }
    }
}

void TA_Cruiser::updateDestroyed() {
    position.x += speed * TA::elapsedTime;
    speed = std::max(0.0F, speed - 0.005F * TA::elapsedTime);
    lockPosition.x += TA::elapsedTime * 1.5F;

    objectSet->getLinks().seaFox->setVelocityAdd({1.5, 0});
    objectSet->getLinks().camera->setLockPosition(lockPosition);
    objectSet->getLinks().camera->forceLockX();

    if(position.x < lockPosition.x - getWidth() - 32) {
        TA::sound::playMusic("sound/lr.vgm");
        objectSet->spawnObject<TA_Transition>(lockPosition + TA_Point(TA::screenWidth + 64, 0),
            lockPosition + TA_Point(TA::screenWidth + 66, TA::screenHeight), 6, false);
        state = State::POST_DESTROYED;
    }

    float newTimer = timer + TA::elapsedTime;
    if(timer < 20 && newTimer >= 20) {
        watcherSprite.setAlpha(0);
        objectSet->spawnObject<TA_DeadKukku>(position + TA_Point(68, 15));
    }
    if(timer < 40 && newTimer >= 40) {
        leftThrowerSprite.setAlpha(0);
        objectSet->spawnObject<TA_DeadKukku>(position + TA_Point(52, 34));
    }
    if(timer < 60 && newTimer >= 60) {
        rightThrowerSprite.setAlpha(0);
        objectSet->spawnObject<TA_DeadKukku>(position + TA_Point(148, 26));
    }

    if(static_cast<int>(newTimer / 4) != static_cast<int>(timer / 4)) {
        constexpr int minX = 16;
        constexpr int maxX = 176;
        constexpr int minY = 32;
        constexpr int maxY = 80;
        int x = minX + TA::random::next() % (maxX - minX + 1);
        int y = minY + TA::random::next() % (maxY - minY + 1);
        objectSet->spawnObject<TA_Explosion>(position + TA_Point(x, y), 0, TA_EXPLOSION_NEUTRAL);
    }

    timer = newTimer;
}

void TA_Cruiser::updatePostDestroyed() {
    lockPosition.x += TA::elapsedTime * 1.5F;
    objectSet->getLinks().seaFox->setVelocityAdd({1.5, 0});
    objectSet->getLinks().camera->setLockPosition(lockPosition);
    objectSet->getLinks().camera->forceLockX();
}
