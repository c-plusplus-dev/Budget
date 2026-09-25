//
// Created by Ayaan Banday on 2025-09-22.
//

#include "Textbox.h"
#include "../HeaderOnly/Helpers.h"

#include <iostream>

Textbox::Textbox() : font {}, fontHeight(0), initialized(false) {}


Textbox::Textbox(int x, int y, int width, int height, TTF_Font* font_) {
    font = font_;
    fontHeight = TTF_GetFontHeight(font_);
    text = "";
    rect = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)};
    initialized = true;
    setMinHeight();
}

Textbox::Textbox(SDL_FRect rect_, TTF_Font* font_) {
    font = font_;
    fontHeight = TTF_GetFontHeight(font_);
    text = "";
    rect = rect_;
    initialized = true;
    setMinHeight();
}

void Textbox::init(int x, int y, int width, int height, TTF_Font *font_) {
    font = font_;
    fontHeight = TTF_GetFontHeight(font_);
    rect = SDL_FRect { static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)};
    initialized = true;
    setMinHeight();
}

void Textbox::init(SDL_FRect rect_, TTF_Font *font_) {
    font = font_;
    fontHeight = TTF_GetFontHeight(font_);
    rect = rect_;
    initialized = true;
    setMinHeight();
}


void Textbox::setMinHeight() {
    if (rect.h < static_cast<float>(fontHeight)) {
        rect.h = static_cast<float>(fontHeight);
    }
}

const char* Textbox::getText() const {
    if (!initialized) {
        SDL_Log("Cannot call getText() on non initialized Textbox");
        return "";
    }
    return text.c_str();
}

void Textbox::setText(const char* txt) {
    if (!initialized) {
        SDL_Log("Cannot call setText() on non initialized Textbox");
        return;
    }
    text = string(txt);
}

void Textbox::draw(SDL_Renderer* renderer) const {
    if (!initialized) {
        SDL_Log("Cannot call draw() on non initialized Textbox");
        return;
    }
    DrawRoundedRect(renderer, 40, 40, 40, &rect, 10);
    int rx = static_cast<int>(rect.x);
    int ry = static_cast<int>(rect.y);
    int rh = static_cast<int>(rect.h);
    int w = renderText(font, renderer, getText(), SDL_Color { 255, 255, 255, 255 }, rx + 5, ry + (rh - fontHeight) / 2);

    if (active) {
        unsigned int ticks = SDL_GetTicks();
        SDL_FRect rct;
        rct.x = static_cast<float>(rx + w + 5);
        rct.y = static_cast<float>(ry + (rh - fontHeight) / 2);
        rct.w = 2;
        rct.h = static_cast<float>(fontHeight);
        if ((ticks / 500) % 2 == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rct);
        }
    }
}

void Textbox::handleEvent(const SDL_Event &event) {
    if (!initialized) {
        SDL_Log("Cannot call handleEvent() on non initialized Textbox");
        return;
    }

    switch (event.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        active = pointInRect(rect, event.motion.x, event.motion.y);
        break;

    case SDL_EVENT_KEY_DOWN:
        if (!active) break;

        // Handle backspace
        if (event.key.key == SDLK_BACKSPACE) {
            deleteCharacter();  // deleteCharacter already handles if the string
                                // is empty.
        }
        break;

    case SDL_EVENT_TEXT_INPUT:
        if (active) {
            text += event.text.text; // append typed characters
        }
        break;

    default:
        break;
    }
}

void Textbox::deleteCharacter() {
    if (text.empty()) return;

    // Move backward until we find the start byte of the last UTF-8 character
    size_t i = text.size() - 1;
    while (i > 0 && (text[i] & 0b11000000) == 0b10000000) {
        // 0b10xxxxxx = continuation byte
        --i;
    }

    text.erase(i); // erase from the start of last code point to end
}

bool Textbox::getInit() const {
    return initialized;
}

void Textbox::setActive(bool active_) {
    active = active_;
}

bool Textbox::getActive() const {
    return active;
}
