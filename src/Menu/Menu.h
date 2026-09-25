//
// Created by Ayaan Banday on 2025-10-25.
//

#ifndef BUDGET_MENU_H
#define BUDGET_MENU_H

#include <vector>
#include <string>
#include <functional>

#define COMMAND 1 << 0
#define SHIFT 1 << 1
#define OPTION 1 << 2
#define CONTROL 2 << 3

struct MenuItem;

struct SubMenu {
    std::vector<MenuItem> items;
};

struct Shortcut {
    int modifiers;
    char key;
};

struct MenuItem {
    std::string title;
    std::function<void()> action;
    SubMenu* menu = nullptr;
    Shortcut shortcut = {};
};

struct Menu {
    std::string title;
    std::vector<MenuItem> items;
    bool mainMenu = false;
};

void CreatePlatformMenu(const std::vector<Menu>& menus);

#endif // BUDGET_MENU_H
