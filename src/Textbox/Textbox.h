//
// Created by Ayaan Banday on 2025-09-22.
//

#ifndef BUDGET_TEXTBOX_H
#define BUDGET_TEXTBOX_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
using namespace std;

class Textbox {
    string text;
    SDL_FRect rect{};
    TTF_Font* font;
    int fontHeight;
    bool initialized;
    bool active = false;
    void setMinHeight();
    void deleteCharacter();
public:
    Textbox();
    Textbox(int x, int y, int width, int height, TTF_Font* font_);
    Textbox(SDL_FRect rect_, TTF_Font* font_);
    void init(int x, int y, int width, int height, TTF_Font* font_);
    void init(SDL_FRect rect_, TTF_Font* font_);
    const char* getText() const;
    void setText(const char* txt);
    void draw(SDL_Renderer* renderer) const;
    void handleEvent(const SDL_Event &event);
    bool getInit() const;
    void setActive(bool active);
    bool getActive() const;
};

#endif // BUDGET_TEXTBOX_H
