//
// Created by Ayaan Banday on 2025-10-27.
//

#ifndef BUDGET_CHECKBOX_H
#define BUDGET_CHECKBOX_H

#include <SDL3/SDL.h>

class CheckBox {
private:
    int x;
    int y;
    int cx;
    int cy;
    int s;
    bool checked;
    void calculateCxCy();
public:
    CheckBox(int x_, int y_, int size);
    void setChecked(bool checked_);
    void draw(SDL_Renderer* renderer) const;
    void setPos(int x_, int y_);
    void setSize(int size);
    void handleEvent(const SDL_Event &event);
    [[nodiscard]] bool isChecked() const;
};

#endif //BUDGET_CHECKBOX_H