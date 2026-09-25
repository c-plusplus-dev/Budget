//
// Created by Ayaan Banday on 2025-10-25.
//

#ifndef BUDGET_PREFERENCES_H
#define BUDGET_PREFERENCES_H
#include <string>
#define MAX_SCROLL_SENSITIVITY 10.0f

using namespace std;

struct Preferences {
    float scrollSensitivity = 5.0f;
    bool scrollInversed = true;
};

int preferences(Preferences &prefs, const string &settingsFile, bool &run, int WIDTH, int HEIGHT);
Preferences loadSettings(const string &filepath);
void saveSettings(Preferences prefs, const string &filepath);

#endif // BUDGET_PREFERENCES_H