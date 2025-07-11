#include "options_section.h"
#include "SDL3/SDL.h"
#include "resource_manager.h"
#include "save.h"

namespace {
    std::string getOptionString(const std::string& name) {
        return TA::resmgr::loadToml("data_select/options_strings.toml").at(name).as_string();
    }
}

class TA_ResolutionOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter("base_height");
        return std::to_string(TA::getBaseHeight(value)) + "p";
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter("base_height");
        value = (value + delta + 4) % 4;
        TA::save::setParameter("base_height", value);
        return TA_MOVE_SOUND_SWITCH;
    }

private:
    std::string name = getOptionString("resolution");
};

class TA_WindowSizeOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int factor = TA::save::getParameter("window_size");
        return "x" + std::to_string(factor);
    }

    TA_MoveSoundId move(int delta) override {
        int factor = TA::save::getParameter("window_size");
        factor += delta;

        TA_MoveSoundId result = TA_MOVE_SOUND_SWITCH;
        if(factor < 1) {
            factor = 1;
            result = TA_MOVE_SOUND_ERROR;
        }
        if(factor > 20) {
            factor = 20;
            result = TA_MOVE_SOUND_ERROR;
        }

        TA::save::setParameter("window_size", factor);
        return result;
    }

private:
    std::string name = getOptionString("window_size");
};

class TA_PixelAROption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter("pixel_ar");
        return (value == 1 ? gg : off);
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter("pixel_ar");
        value = 1 - value;
        TA::save::setParameter("pixel_ar", value);
        return TA_MOVE_SOUND_SWITCH;
    }

private:
    std::string name = getOptionString("pixel_ar");
    std::string off = getOptionString("off");
    std::string gg = getOptionString("pixel_ar_gg");
};

class TA_VSyncOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter("vsync");
        switch(value) {
            case 0:
                return off;
            case 1:
                return on;
            case 2:
                return adapt;
            default:
                return "";
        }
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter("vsync");
        value = (value + 1) % 3;
        TA::save::setParameter("vsync", value);
        SDL_SetRenderVSync(TA::renderer, (value == 2 ? -1 : value));
        return TA_MOVE_SOUND_SWITCH;
    }

private:
    std::string name = getOptionString("vsync");
    std::string on = getOptionString("on");
    std::string off = getOptionString("off");
    std::string adapt = getOptionString("vsync_adapt");
};

class TA_ScaleModeOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter("scale_mode");
        return (value == 1 ? smooth : sharp);
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter("scale_mode");
        value = 1 - value;
        TA::save::setParameter("scale_mode", value);
        return TA_MOVE_SOUND_SWITCH;
    }

private:
    std::string name = getOptionString("scale_mode");
    std::string smooth = getOptionString("scale_mode_smooth");
    std::string sharp = getOptionString("scale_mode_sharp");
};

class TA_VolumeOption : public TA_Option {
    std::string name, param;

public:
    TA_VolumeOption(std::string name, std::string param) {
        this->name = name;
        this->param = param;
    }

    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter(param);
        std::string bar(10, 'E');
        bar[0] = 'C';
        bar[9] = 'F';
        for(int i = 1; i <= value; i++) {
            bar[i] = 'D';
        }
        return bar;
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter(param);
        value += delta;

        TA_MoveSoundId result = TA_MOVE_SOUND_SWITCH;
        if(value < 0) {
            value = 0;
            result = TA_MOVE_SOUND_ERROR;
        }
        if(value > 8) {
            value = 8;
            result = TA_MOVE_SOUND_ERROR;
        }

        TA::save::setParameter(param, value);
        return result;
    }

    constexpr bool hasNegativeMove() override { return true; }
};

class TA_MapKeyboardOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        if(!locked) {
            return "";
        }
        return getOptionString("button_" + buttons[button]);
    }

    bool isLocked() override { return locked; }
    void unlock() override { locked = false; }

    TA_MoveSoundId move(int delta) override {
        locked = true;
        button = 0;
        return TA_MOVE_SOUND_SELECT;
    }

    TA_MoveSoundId updateLocked() override {
        for(int scancode = 0; scancode < SDL_SCANCODE_COUNT; scancode++) {
            if(scancode == SDL_SCANCODE_ESCAPE || scancode == SDL_SCANCODE_RALT) {
                continue;
            }
            if(TA::keyboard::isScancodeJustPressed(SDL_Scancode(scancode))) {
                TA::save::setParameter("keyboard_map_" + buttons[button], scancode);
                button++;
                if(button >= (int)buttons.size()) {
                    locked = false;
                    TA::save::writeToFile();
                }
                return TA_MOVE_SOUND_SWITCH;
            }
        }

        return TA_MOVE_SOUND_EMPTY;
    }

private:
    const std::array<std::string, 9> buttons{"up", "down", "left", "right", "a", "b", "lb", "rb", "start"};

    std::string name = getOptionString("map_keyboard");
    bool locked = false;
    int button = 0;
};

class TA_MapGamepadOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        if(!locked) {
            return "";
        }
        return getOptionString("button_" + buttons[button]);
    }

    bool isLocked() override { return locked; }
    void unlock() override { locked = false; }

    TA_MoveSoundId move(int delta) override {
        locked = true;
        button = 0;
        return TA_MOVE_SOUND_SELECT;
    }

    TA_MoveSoundId updateLocked() override {
        for(int id = 0; id < SDL_GAMEPAD_BUTTON_COUNT; id++) {
            if(TA::gamepad::isControllerButtonJustPressed(SDL_GamepadButton(id))) {
                TA::save::setParameter("gamepad_map_" + buttons[button], id);
                button++;
                if(button >= (int)buttons.size()) {
                    locked = false;
                    TA::save::writeToFile();
                }
                return TA_MOVE_SOUND_SWITCH;
            }
        }

        return TA_MOVE_SOUND_EMPTY;
    }

private:
    const std::array<std::string, 5> buttons{"a", "b", "lb", "rb", "start"};

    std::string name = getOptionString("map_gamepad");
    bool locked = false;
    int button = 0;
};

class TA_HideOnscreenOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter("hide_onscreen");
        return (value ? on : off);
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter("hide_onscreen");
        value = 1 - value;
        TA::save::setParameter("hide_onscreen", value);
        return TA_MOVE_SOUND_SWITCH;
    }

private:
    std::string name = getOptionString("hide_onscreen");
    std::string on = getOptionString("on");
    std::string off = getOptionString("off");
};

class TA_RumbleOption : public TA_Option {
public:
    std::string getName() override { return name; }

    std::string getValue() override {
        int value = TA::save::getParameter("rumble");
        return (value ? on : off);
    }

    TA_MoveSoundId move(int delta) override {
        int value = TA::save::getParameter("rumble");
        value = 1 - value;
        TA::save::setParameter("rumble", value);
        return TA_MOVE_SOUND_SWITCH;
    }

private:
    std::string name = getOptionString("rumble");
    std::string on = getOptionString("on");
    std::string off = getOptionString("off");
};

void TA_OptionsSection::load() {
    font.loadFont("fonts/pause_menu.toml");

    groups = std::vector<std::string>(3);
    groups[0] = getOptionString("video");
    groups[1] = getOptionString("controls");
    groups[2] = getOptionString("sound");

    options.resize(groups.size());
    options[0].push_back(std::make_unique<TA_ResolutionOption>());
    options[0].push_back(std::make_unique<TA_PixelAROption>());
    options[0].push_back(std::make_unique<TA_ScaleModeOption>());
    options[0].push_back(std::make_unique<TA_VSyncOption>());
#ifndef __ANDROID__
    options[0].push_back(std::make_unique<TA_WindowSizeOption>());
#endif

    options[1].push_back(std::make_unique<TA_MapKeyboardOption>());
    options[1].push_back(std::make_unique<TA_MapGamepadOption>());
#ifdef __ANDROID__
    options[1].push_back(std::make_unique<TA_HideOnscreenOption>());
#endif
    options[1].push_back(std::make_unique<TA_RumbleOption>());

    options[2].push_back(std::make_unique<TA_VolumeOption>(getOptionString("main"), "main_volume"));
    options[2].push_back(std::make_unique<TA_VolumeOption>(getOptionString("music"), "music_volume"));
    options[2].push_back(std::make_unique<TA_VolumeOption>(getOptionString("sfx"), "sfx_volume"));

    switchSound.load("sound/switch.ogg", TA_SOUND_CHANNEL_SFX1);
    selectSound.load("sound/select_item.ogg", TA_SOUND_CHANNEL_SFX2);
    backSound.load("sound/select.ogg", TA_SOUND_CHANNEL_SFX2);
    errorSound.load("sound/damage.ogg", TA_SOUND_CHANNEL_SFX3);

    float y = 32;
    for(int pos = 0; pos < 5; pos++) {
        buttons[pos][0].setRectangle({(float)getLeftX() - 32, y}, {(float)getLeftX() + 80, y + 17});
        buttons[pos][1].setRectangle({(float)getLeftX() + 80, y}, {(float)getLeftX() + 192, y + 17});
        y += 20;
    }

    backButton.setRectangle({0, 0}, {128, 32});
}

TA_MainMenuState TA_OptionsSection::update() {
    backButton.update();
    for(int pos = 0; pos < 5; pos++) {
        buttons[pos][0].update();
        buttons[pos][1].update();
    }

    if(listTransitionTimeLeft > 0) {
        return TA_MAIN_MENU_OPTIONS;
    }

    switch(state) {
        case STATE_SELECTING_GROUP:
            updateGroupSelector();
            break;
        case STATE_SELECTING_OPTION:
            updateOptionSelector();
            break;
        case STATE_QUIT:
            state = STATE_SELECTING_GROUP;
            return TA_MAIN_MENU_DATA_SELECT;
    }

    return TA_MAIN_MENU_OPTIONS;
}

void TA_OptionsSection::updateGroupSelector() {
    if(controller->isJustChangedDirection()) {
        TA_Direction direction = controller->getDirection();
        if(direction == TA_DIRECTION_UP && group > 0) {
            group--;
            switchSound.play();
        } else if(direction == TA_DIRECTION_DOWN && group < static_cast<int>(groups.size()) - 1) {
            group++;
            switchSound.play();
        }
    }

    bool selected = false;
    for(int pos = 0; pos < static_cast<int>(options.size()); pos++) {
        if(buttons[pos][0].isReleased() || buttons[pos][1].isReleased()) {
            group = pos;
            selected = true;
        }
    }

    if(selected || controller->isJustPressed(TA_BUTTON_A)) {
        listTransitionTimeLeft = listTransitionTime * 2;
        option = 0;
        selectSound.play();
        return;
    }

    if(controller->isJustPressed(TA_BUTTON_B) || backButton.isReleased()) {
        state = STATE_QUIT;
        backSound.play();
    }
}

void TA_OptionsSection::updateOptionSelector() {
    if(options[group][option]->isLocked()) {
        if(backButton.isReleased()) {
            options[group][option]->unlock();
            backSound.play();
            return;
        }
        playMoveSound(options[group][option]->updateLocked());
        return;
    }

    TA_MoveSoundId sound = TA_MOVE_SOUND_EMPTY;

    if(controller->isJustChangedDirection()) {
        TA_Direction direction = controller->getDirection();

        switch(direction) {
            case TA_DIRECTION_UP:
                if(option > 0) {
                    option--;
                    sound = TA_MOVE_SOUND_SWITCH;
                }
                break;
            case TA_DIRECTION_DOWN:
                if(option < (int)options[group].size() - 1) {
                    option++;
                    sound = TA_MOVE_SOUND_SWITCH;
                }
                break;
            case TA_DIRECTION_LEFT:
                sound = options[group][option]->move(-1);
                TA::save::writeToFile();
                break;
            case TA_DIRECTION_RIGHT:
                sound = options[group][option]->move(1);
                TA::save::writeToFile();
                break;
            default:
                break;
        }
    }

    for(int pos = 0; pos < static_cast<int>(options[group].size()); pos++) {
        if(options[group][pos]->hasNegativeMove()) {
            if(buttons[pos][0].isReleased()) {
                option = pos;
                sound = options[group][pos]->move(-1);
                TA::save::writeToFile();
            }
            if(buttons[pos][1].isReleased()) {
                option = pos;
                sound = options[group][pos]->move(1);
                TA::save::writeToFile();
            }
        } else if(buttons[pos][0].isReleased() || buttons[pos][1].isReleased()) {
            option = pos;
            sound = options[group][pos]->move(1);
            TA::save::writeToFile();
        }
    }

    if(controller->isJustPressed(TA_BUTTON_A)) {
        sound = options[group][option]->move(1);
        TA::save::writeToFile();
    }

    playMoveSound(sound);

    if(controller->isJustPressed(TA_BUTTON_B) || backButton.isReleased()) {
        listTransitionTimeLeft = listTransitionTime * 2;
        backSound.play();
    }
}

void TA_OptionsSection::playMoveSound(TA_MoveSoundId id) {
    switch(id) {
        case TA_MOVE_SOUND_SWITCH:
            switchSound.play();
            break;
        case TA_MOVE_SOUND_SELECT:
            selectSound.play();
            break;
        case TA_MOVE_SOUND_ERROR:
            errorSound.play();
            break;
        default:
            break;
    }
}

void TA_OptionsSection::draw() {
    updateAlpha();

    if(state == STATE_SELECTING_OPTION) {
        drawOptionList();
    } else {
        drawGroupList();
    }
}

void TA_OptionsSection::drawGroupList() {
    TA_Point textPosition{(float)getLeftX() + 16, 36};
    bool touchscreen = controller->isTouchscreen();

    for(int pos = 0; pos < (int)groups.size(); pos++) {
        if((!touchscreen && pos == group) ||
            (touchscreen && (buttons[pos][0].isPressed() || buttons[pos][1].isPressed()))) {
            drawHighlight(textPosition.y - 3);
        }
        font.drawText(textPosition, groups[pos]);
        textPosition.y += 20;
    }
}

void TA_OptionsSection::drawOptionList() {
    float lx = getLeftX() + 16, rx = getLeftX() + 144, y = 36;
    bool touchscreen = controller->isTouchscreen();

    for(int pos = 0; pos < (int)options[group].size(); pos++) {
        std::string left = options[group][pos]->getName();
        std::string right = options[group][pos]->getValue();
        int offset = font.getTextWidth(right, {-1, 0});

        if(touchscreen) {
            if(options[group][pos]->hasNegativeMove()) {
                if(buttons[pos][0].isPressed()) {
                    drawHighlight(lx - 28, y - 3, 25);
                }
                if(buttons[pos][1].isPressed()) {
                    drawHighlight(rx + 4, y - 3, 25);
                }
            } else if(buttons[pos][0].isPressed() || buttons[pos][1].isPressed()) {
                drawHighlight(y - 3);
            }
        } else if(pos == option) {
            drawHighlight(y - 3);
        }

        font.drawText({lx, y}, left, {-1, 0});
        font.drawText({rx - offset, y}, right, {-1, 0});
        if(touchscreen && options[group][pos]->hasNegativeMove()) {
            font.drawText({lx - 20, y}, "-");
            font.drawText({rx + 12, y}, "+");
        }

        y += 20;
    }
}

void TA_OptionsSection::drawHighlight(float x, float y, float width) {
    SDL_FRect rect = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), 15};

    for(int num = 0; num < 4; num++) {
        SDL_SetRenderDrawColor(
            TA::renderer, num * 28 * alpha / 255, num * 24 * alpha / 255, num * 28 * alpha / 255, 255);

        SDL_FRect targetRect = rect;
        targetRect.x *= static_cast<float>(TA::scaleFactor);
        targetRect.y *= static_cast<float>(TA::scaleFactor);
        targetRect.w *= static_cast<float>(TA::scaleFactor);
        targetRect.h *= static_cast<float>(TA::scaleFactor);

        SDL_RenderFillRect(TA::renderer, &targetRect);
        rect.x += 2;
        rect.w -= 4;
    }
}

void TA_OptionsSection::drawHighlight(float y) {
    drawHighlight(getLeftX() + 4, y, 152);
}

void TA_OptionsSection::updateAlpha() {
    alpha = baseAlpha;

    if(listTransitionTimeLeft > 0) {
        bool flag1 = (listTransitionTimeLeft > listTransitionTime);
        listTransitionTimeLeft -= TA::elapsedTime;
        bool flag2 = (listTransitionTimeLeft > listTransitionTime);

        if(flag1 != flag2) {
            state = (state == STATE_SELECTING_GROUP ? STATE_SELECTING_OPTION : STATE_SELECTING_GROUP);
        }
        if(listTransitionTimeLeft > listTransitionTime) {
            alpha *= (listTransitionTimeLeft - listTransitionTime) / listTransitionTime;
        } else {
            alpha *= (1 - listTransitionTimeLeft / listTransitionTime);
        }
    }

    font.setAlpha(alpha);
}
