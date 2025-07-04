#include "sea_fox.h"
#include "anti_air_missile.h"
#include "bullet.h"
#include "controller.h"
#include "hud.h"
#include "object_set.h"
#include "ring.h"
#include "save.h"

void TA_SeaFox::load(TA_Links links) {
    this->links = links;

    loadFromToml("tails/seafox.toml");
    setCamera(links.camera);
    setAnimation("idle");

    jumpSound.load("sound/jump.ogg", TA_SOUND_CHANNEL_SFX1);
    damageSound.load("sound/damage.ogg", TA_SOUND_CHANNEL_SFX1);
    bulletSound.load("sound/bullet.ogg", TA_SOUND_CHANNEL_SFX3);
    extraSpeedSound.load("sound/extra_speed.ogg", TA_SOUND_CHANNEL_SFX3);

    hitbox.setRectangle(TA_Point(9, 4), TA_Point(23, 30));
}

void TA_SeaFox::setSpawnPoint(TA_Point position, bool flip) {
    this->position = position;
    this->flip = this->neededFlip = flip;
    updateFollowPosition();
    links.camera->setFollowPosition(&followPosition);
}

void TA_SeaFox::update() {
    if(dead) {
        updateDead();
        setFlip(flip);
        return;
    }

    physicsStep();

    updateDirection();
    updateFollowPosition();
    updateDrill();
    updateItem();
    updateDamage();

    setFlip(flip);
}

void TA_SeaFox::physicsStep() {
    auto updateSpeed = [](float& currentSpeed, float neededSpeed, float drag) {
        if(currentSpeed > neededSpeed) {
            currentSpeed = std::max(neededSpeed, currentSpeed - drag * TA::elapsedTime);
        } else {
            currentSpeed = std::min(neededSpeed, currentSpeed + drag * TA::elapsedTime);
        }
    };

    bool underwater = (position.y + 30 > waterLevel);

    if(links.controller->getDirection() != TA_DIRECTION_MAX) {
        TA_Point vector = links.controller->getDirectionVector();
        updateSpeed(velocity.x, vector.x + (vector.x > 0 && extraSpeed ? 1.0F : 0.0F), inputDrag);
        if(underwater) {
            updateSpeed(velocity.y, vector.y, inputDrag);
        }
    } else {
        updateSpeed(velocity.x, 0, (groundMode ? inputDrag : horizontalDrag));
        if(underwater) {
            updateSpeed(velocity.y, 0, (groundMode ? inputDrag : verticalDrag));
        }
    }

    if(!underwater) {
        velocity.y = std::min(float(1), velocity.y + deadGravity * TA::elapsedTime);
    }

    if(groundMode) {
        if(ground) {
            velocity.y = 0;
            setVelocityAdd({-0.5, 0});
            if(links.controller->isJustPressed(TA_BUTTON_A)) {
                if(extraSpeed) {
                    extraSpeedSound.fadeOut(0);
                    extraSpeed = false;
                    extraSpeedReleaseTime = extraSpeedTimer;
                }
                jumpSound.play();
                jumpSpeed = initJumpSpeed;
                if(extraSpeedTimer < extraSpeedReleaseTime + extraSpeedAddTime) {
                    jumpSpeed += extraSpeedAddYSpeed * std::min(1.0F, extraSpeedReleaseTime / extraSpeedTime);
                    extraSpeedTimer = extraSpeedReleaseTime + extraSpeedAddTime + 1;
                }
                velocity.y = std::min(maxYSpeed, std::max(minJumpSpeed, jumpSpeed));
                jump = true;
                ground = false;
                jumpReleased = false;
            }
        } else {
            if(jump) {
                if(!jumpReleased && !links.controller->isPressed(TA_BUTTON_A)) {
                    jumpReleased = true;
                }
                if(jumpReleased) {
                    jumpSpeed = std::max(jumpSpeed, releaseJumpSpeed);
                }
                jumpSpeed += gravity * TA::elapsedTime;
                velocity.y = std::min(maxYSpeed, std::max(minJumpSpeed, jumpSpeed));
            } else {
                velocity.y = std::min(maxYSpeed, velocity.y + gravity * TA::elapsedTime);
            }
        }
    }

    if(groundMode) {
        int flags =
            moveAndCollide(TA_Point(9, 4), TA_Point(23, 30), (velocity + velocityAdd) * TA::elapsedTime, ground);
        ground = (flags & TA_GROUND_COLLISION) != 0;
    } else {
        moveAndCollide(TA_Point(9, 4), TA_Point(23, 30), (velocity + velocityAdd) * TA::elapsedTime, false);
    }

    setPosition(position);
    velocityAdd = {0, 0};

    // TA::printLog("%f %f", position.x, position.y);
}

bool TA_SeaFox::checkPawnCollision(TA_Rect& hitbox) {
    int flags = links.objectSet->checkCollision(hitbox);
    if(flags & TA_COLLISION_SOLID) {
        return true;
    }
    return false;
}

void TA_SeaFox::updateDirection() {
    if(flip != neededFlip) {
        if(!isAnimated()) {
            setAnimation("idle");
            flip = neededFlip;
        }
    } else if(links.controller->isJustPressed(TA_BUTTON_A) && !groundMode) {
        setAnimation("rotate");
        neededFlip = !flip;
    }
}

void TA_SeaFox::updateFollowPosition() {
    followPosition =
        position + TA_Point(getWidth() / 2, getHeight() / 2) - TA_Point(TA::screenWidth / 2, TA::screenHeight / 2);
    followPosition.x += (flip ? -1 : 1) * (TA::screenWidth * 0.15);
}

void TA_SeaFox::updateDrill() {
    if(flip) {
        drillHitbox.setRectangle(TA_Point(2, 10), TA_Point(16, 24));
    } else {
        drillHitbox.setRectangle(TA_Point(24, 10), TA_Point(30, 24));
    }
    drillHitbox.setPosition(position);
}

void TA_SeaFox::updateItem() {
    if(extraSpeedTimer < extraSpeedReleaseTime + extraSpeedAddTime) {
        extraSpeedTimer += TA::elapsedTime;
    }
    if(extraSpeed) {
        if(!links.controller->isPressed(TA_BUTTON_B) || links.hud->getCurrentItem() != ITEM_EXTRA_SPEED ||
            !TA::sound::isPlaying(TA_SOUND_CHANNEL_SFX3)) {
            extraSpeedSound.fadeOut(0);
            extraSpeed = false;
            extraSpeedReleaseTime = extraSpeedTimer;
        }
    }
    if(vulcanGunTimer < vulcanGunTime) {
        updateVulcanGun();
        return;
    }
    if(!links.controller->isJustPressed(TA_BUTTON_B)) {
        return;
    }

    switch(links.hud->getCurrentItem()) {
        case ITEM_VULCAN_GUN:
            vulcanGunTimer = 0;
            break;
        case ITEM_ANTI_AIR_MISSILE:
            if(!links.objectSet->hasCollisionType(TA_COLLISION_BOMB)) {
                links.objectSet->spawnObject<TA_AntiAirMissile>(position + TA_Point(8, -12));
            }
            break;
        case ITEM_EXTRA_SPEED:
            if(ground) {
                extraSpeedSound.play();
                extraSpeed = true;
                extraSpeedTimer = 0;
            } else {
                damageSound.play();
            }
            break;
        default:
            damageSound.play();
            break;
    }
}

void TA_SeaFox::updateVulcanGun() {
    int prev = vulcanGunTimer / vulcanGunInterval;
    vulcanGunTimer += TA::elapsedTime;
    int cur = vulcanGunTimer / vulcanGunInterval;

    if(prev != cur) {
        TA_Point bulletPosition = position + TA_Point((flip ? 0 : 26), 20);
        TA_Point bulletVelocity = TA_Point((flip ? -5 : 5), 0);
        links.objectSet->spawnObject<TA_VulcanGunBullet>(bulletPosition, bulletVelocity);
        bulletSound.play();
    }
}

void TA_SeaFox::updateDamage() {
    hitbox.setPosition(position);
    if(invincibleTimer < invincibleTime) {
        invincibleTimer += TA::elapsedTime;
        setAlpha(180);
        return;
    }
    setAlpha(255);

    if(links.objectSet->checkCollision(hitbox) & TA_COLLISION_DAMAGE) {
        if(TA::save::getParameter("ring_drop")) {
            dropRings();
            links.objectSet->addRings(-4);
        } else {
            links.objectSet->addRings(-2);
        }
        TA::gamepad::rumble(0.75, 0.75, 20);
        if(TA::save::getSaveParameter("rings") <= 0) {
            TA::sound::playMusic("sound/death.vgm", 0);
            setAnimation("dead");
            dead = true;
        } else {
            velocity = velocity * -1;
            invincibleTimer = 0;
            damageSound.play();
        }
    }
}

void TA_SeaFox::dropRings() {
    if(TA::save::getSaveParameter("rings") <= 4) {
        return;
    }
    TA_Point ringPosition = position + TA_Point(20, 20);
    links.objectSet->spawnObject<TA_Ring>(ringPosition, TA_Point(1.5, -1), 64);
    links.objectSet->spawnObject<TA_Ring>(ringPosition, TA_Point(-1.5, -1), 64);
    links.objectSet->spawnObject<TA_Ring>(ringPosition, TA_Point(0.75, -2), 64);
    links.objectSet->spawnObject<TA_Ring>(ringPosition, TA_Point(-0.75, -2), 64);
}

void TA_SeaFox::updateDead() {
    velocity.x = 0;
    velocity.y += deadGravity * TA::elapsedTime;
    position = position + velocity * TA::elapsedTime;
    setPosition(position);

    deadTimer += TA::elapsedTime;
    flip = (getAnimationFrame() >= 3);
}
