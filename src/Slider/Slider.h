//
// Created by Ayaan Banday on 2025-10-26.
//

#ifndef BUDGET_SLIDER_H
#define BUDGET_SLIDER_H

#include <SDL3/SDL.h>

struct Point {
    float x;
    float y;
};

class Slider {
private:
    Point start{};
    Point end{};
    float value;
    bool heldOn;
    [[nodiscard]] Point lerpPositions() const;
public:
    Slider(float x1, float y1, float x2, float y2, float start_value);
    void setStartPoint(float x, float y);
    void setEndPoint(float x, float y);
    void setValue(float value_);
    void getValue(float &value_) const;
    [[nodiscard]] float getValue() const;
    void draw(SDL_Renderer* renderer, SDL_Color color) const;
    void handleEvent(const SDL_Event &event);
};

#endif //BUDGET_SLIDER_H