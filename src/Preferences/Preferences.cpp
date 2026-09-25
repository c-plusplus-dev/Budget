//
// Created by Ayaan Banday on 2025-10-25.
//

#include "Preferences.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include <fstream>

#include "../CheckBox/CheckBox.h"
#include "../HeaderOnly/Helpers.h"
#include "../HeaderOnly/json.hpp"
#include "../Slider/Slider.h"
//#include "../Textbox/Textbox.h"

using namespace std;
using namespace nlohmann;

void snapSlider(Slider &slider, float maximum) {
    float value = slider.getValue();
    maximum -= 1; // I don't know how but it fixes a bug
    value *= maximum;
    slider.setValue(roundf(value) / maximum);
}

Preferences loadSettings(const string &filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        return {}; // default
    }
    json settings;
    file >> settings;
    file.close();

    Preferences prefs {};
    try {
        prefs.scrollSensitivity = settings["scrollSensitivity"];
        prefs.scrollInversed = settings["scrollInversed"];
    }
    catch (detail::type_error&) { // often means a new setting has been added
        return {};
    }

    return prefs;
}

void saveSettings(Preferences prefs, const string &filepath) {
    json settings;
    settings["scrollSensitivity"] = prefs.scrollSensitivity;
    settings["scrollInversed"] = prefs.scrollInversed;

    ofstream file(filepath);
    if (!file.is_open()) {
        return;
    }

    file << settings.dump(4);
    file.close();
}

int preferences(Preferences &prefs, const string &settingsFile, bool &run, int WIDTH, int HEIGHT) {
    prefs = loadSettings(settingsFile);

    // WIDTH and HEIGHT are copies
    WIDTH -= 100;
    HEIGHT -= 100;

    SDL_Window* window = SDL_CreateWindow("Preferences", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        return 1;
    }

    TTF_Font* font = loadFont(18, renderer);

    float scrollSensitivity = prefs.scrollSensitivity;
    Slider scrollSensitivitySlider {
        0,
        0,
        0,
        0,
        scrollSensitivity / MAX_SCROLL_SENSITIVITY
    };

    bool scrollInversed = prefs.scrollInversed;
    CheckBox scrollInversedCheckBox {20, 45, 20};
    scrollInversedCheckBox.setChecked(scrollInversed);

    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);

    SDL_Event event;
    run = true;
    SDL_StartTextInput(window);
    while (run) {
        while (SDL_PollEvent(&event)) {
            scrollSensitivitySlider.handleEvent(event);
            snapSlider(scrollSensitivitySlider, MAX_SCROLL_SENSITIVITY);
            scrollInversedCheckBox.handleEvent(event);
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    run = false;
                    break;
                }
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    if (event.window.windowID == SDL_GetWindowID(window)) run = false;
                    break;
                }
                default:
                    break;
            }
        }
        scrollSensitivity = roundf(scrollSensitivitySlider.getValue() * (MAX_SCROLL_SENSITIVITY - 1)) + 1;
        scrollInversed = scrollInversedCheckBox.isChecked();

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        int w = renderText(font, renderer, fmt("Scroll Sensitivity: %02d ", static_cast<int>(scrollSensitivity)).c_str(), white, 10, 10);

        scrollSensitivitySlider.setStartPoint(static_cast<float>(w) + 10.0f, 24);
        scrollSensitivitySlider.setEndPoint(static_cast<float>(w) + 110.0f, 24);
        scrollSensitivitySlider.draw(renderer, white);

        scrollInversedCheckBox.draw(renderer);
        renderTextCenteredY(font, renderer, "Scroll Inversed", white, 35, 45);

        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }
    SDL_StopTextInput(window);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    prefs.scrollSensitivity = scrollSensitivity;
    prefs.scrollInversed = scrollInversed;

    saveSettings(prefs, settingsFile);
    return 0;
}
