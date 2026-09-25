//
// Created by Ayaan Banday on 2025-10-27.
//

#ifndef BUDGET_SCROLLBAR_H
#define BUDGET_SCROLLBAR_H

#include <SDL3/SDL.h>

class ScrollBar {
private:
    bool draggingScrollbar = false;
    float dragOffset = 0.0f;
public:
    float scrollY;
    float scrollYVelocity;
    float totalContentHeight = 0;
    float totalViewableHeight = 0;
    float maxScrollY = 0;
    float scrollSensitivity = 5;
    bool scrollInversed = false;
    float scrollFriction;
    float height;
    float x, y;
    ScrollBar(float x_, float y_, float height_, float scrollSensitivity_, bool scrollInversed_, float scrollFriction_);
    void setScrollY(float scrollY_);
    [[nodiscard]] float getScrollY() const;
    void draw(SDL_Renderer* renderer, float maxScrollY_, float totalContentHeight_, float totalViewableHeight_);
    void handleEvent(const SDL_Event &event);
    void tick();
};


#endif //BUDGET_SCROLLBAR_H