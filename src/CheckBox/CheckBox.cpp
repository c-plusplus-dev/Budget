//
// Created by Ayaan Banday on 2025-10-27.
//

#include "CheckBox.h"

#include "../HeaderOnly/Helpers.h"

void CheckBox::calculateCxCy() {
    cx = x - s / 2;
    cy = y - s / 2;
}

CheckBox::CheckBox(const int x_, const int y_, const int size) {
    x = x_;
    y = y_;
    s = size;
    calculateCxCy();
    checked = false;
}

void CheckBox::setChecked(const bool checked_) {
    checked = checked_;
}

void CheckBox::draw(SDL_Renderer* renderer) const {
    DrawRoundedRect(renderer, 255, 255, 255, cx, cy, s, s, 5);
    if (!checked) {
        DrawRoundedRect(renderer, 20, 20, 20, cx + 1, cy + 1, s - 2, s - 2, 5);
        return;
    }
    DrawRoundedRect(renderer, 0, 0, 255, cx + 1, cy + 1, s - 2, s - 2, 5);
}

void CheckBox::setPos(const int x_, const int y_) {
    x = x_;
    y = y_;
    calculateCxCy();
}

void CheckBox::setSize(const int size) {
    s = size;
    calculateCxCy();
}

void CheckBox::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            SDL_FRect rect;
            rect.x = static_cast<float>(x);
            rect.y = static_cast<float>(y);
            rect.w = s;
            rect.h = s;
            if (!pointInRect(rect, event.button.x, event.button.y)) {
                break;
            }
            checked = !checked;
        }
        default: {
            break;
        }
    }
}

bool CheckBox::isChecked() const {
    return checked;
}
