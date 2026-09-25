//
// Created by Ayaan Banday on 2025-10-26.
//

#include "Slider.h"
#include "../HeaderOnly/Helpers.h"
#include <cmath>

Point Slider::lerpPositions() const {
    const float x = lerp(start.x, end.x, value);
    const float y = lerp(start.y, end.y, value);
    return {x, y};
}

Slider::Slider(const float x1, const float y1, const float x2, const float y2, float start_value) {
    start = {x1, y1};
    end = {x2, y2};
    value = start_value;
    heldOn = false;
}

void Slider::setStartPoint(const float x, const float y) {
    start = {x, y};
}

void Slider::setEndPoint(const float x, const float y) {
    end = {x, y};
}

void Slider::setValue(const float value_) {
    value = ::clamp(value_, 0.0f, 1.0f);
}

void Slider::getValue(float &value_) const {
    value_ = value;
}

float Slider::getValue() const {
    return value;
}

void Slider::draw(SDL_Renderer* renderer, SDL_Color color) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    DrawThickLineHV(renderer, static_cast<int>(start.x), static_cast<int>(start.y), static_cast<int>(end.x), static_cast<int>(end.y), 2);
    auto [x, y] = lerpPositions();
    DrawCircle(renderer, static_cast<int>(x), static_cast<int>(y), 5);
}

void Slider::handleEvent(const SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (event.button.button != SDL_BUTTON_LEFT) {
                break;
            }
            const float x = event.button.x;
            const float y = event.button.y;
            if (start.x > x || x > end.x || ::distance(x, y, x, start.y) > 6) {
                break;
            }
            heldOn = true;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            if (!heldOn) {
                break;
            }
            float x = event.motion.x; // only x for now
            //float y = event.motion.y - offsetY;
            setValue(ilerp(start.x, end.x, x));
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            heldOn = false;
            break;
        }
        default: {
            break;
        }
    }
}
