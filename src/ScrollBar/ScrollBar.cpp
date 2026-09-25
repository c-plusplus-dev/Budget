//
// Created by Ayaan Banday on 2025-10-27.
//

#include "ScrollBar.h"
#include "../HeaderOnly/Helpers.h"

ScrollBar::ScrollBar(float x_, float y_, float height_, float scrollSensitivity_, bool scrollInversed_, float scrollFriction_) {
    x = x_;
    y = y_;
    height = height_;
    scrollY = 0;
    scrollYVelocity = 0;
    scrollSensitivity = scrollSensitivity_;
    scrollInversed = scrollInversed_;
    scrollFriction = scrollFriction_;
}

void ScrollBar::setScrollY(float scrollY_) {
    scrollY = scrollY_;
}

float ScrollBar::getScrollY() const {
    return scrollY;
}

void ScrollBar::draw(SDL_Renderer* renderer, float maxScrollY_, float totalContentHeight_, float totalViewableHeight_) {
    maxScrollY = maxScrollY_;
    totalContentHeight = totalContentHeight_;
    totalViewableHeight = totalViewableHeight_;
    DrawScrollbar(renderer, static_cast<int>(x), static_cast<int>(y), static_cast<int>(height), scrollY / maxScrollY, totalContentHeight / totalViewableHeight);
}

void ScrollBar::handleEvent(const SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_WHEEL: {
            scrollYVelocity = event.wheel.y * scrollSensitivity * (scrollInversed ? -1.0f : 1.0f);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            float mouse_x = event.button.x, mouse_y = event.button.y;

            float scrollRange = totalContentHeight / totalViewableHeight;
            float scrollPosition = scrollY / maxScrollY;
            unsigned long temp = CalculateThumbHeightAndY(static_cast<int>(y), static_cast<int>(height), scrollPosition, scrollRange);
            int barHeight = DecombineIntx2(temp, 0);
            int barY = DecombineIntx2(temp, 1);
            SDL_FRect bar = { x - 5.0f, static_cast<float>(barY), 10.0f, static_cast<float>(barHeight) };

            if (pointInRect(bar, mouse_x, mouse_y)) {
                draggingScrollbar = true;
                dragOffset = mouse_y - bar.y;
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            if (draggingScrollbar) {
                float mouse_y = event.motion.y;
                float trackTop = y;
                float trackHeight = height;
                float trackBottom = trackTop + trackHeight;

                float scrollRange = totalContentHeight / totalViewableHeight;
                float scrollPosition = scrollY / maxScrollY;
                unsigned long temp = CalculateThumbHeightAndY(static_cast<int>(y), static_cast<int>(height), scrollPosition, scrollRange);
                auto barHeight = static_cast<float>(DecombineIntx2(temp, 0));

                float newBarY = mouse_y - dragOffset;
                newBarY = std::clamp(newBarY, trackTop, trackBottom - barHeight);

                float thumbRatio = (newBarY - trackTop) / (trackHeight - barHeight);
                scrollY = thumbRatio * maxScrollY;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            draggingScrollbar = false;
            dragOffset = 0;
            break;
        }
        default:
            break;
    }
}

void ScrollBar::tick() {
    scrollY += scrollYVelocity;
    scrollYVelocity *= scrollFriction;
    scrollY = std::clamp(scrollY, 0.0f, maxScrollY);
}
