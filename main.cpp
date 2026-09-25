#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#define NANOSVG_IMPLEMENTATION
#include "src/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include <__chrono/year_month_day.h>

#include "src/nanosvgrast.h"

#include "src/HeaderOnly/Block.h"
#include "src/HeaderOnly/Helpers.h"
#include "src/Textbox/Textbox.h"
#include "src/HeaderOnly/Schedule.h"
#include "src/Menu/Menu.h"
#include "src/Preferences/Preferences.h"
#include "src/ScrollBar/ScrollBar.h"

typedef unsigned int display_mode;

#define DEFAULT 0
#define PIE     1
#define VIEW    2
#define GRAPH   3

struct BlockNameAndPercentColorMix {
    char* block_name;
    double percent;
    SDL_Color color;
    int kr;
};

void SortBlockNameAndPercentColorMixByPercent(std::vector<BlockNameAndPercentColorMix>& bnapcm) {
    std::sort(bnapcm.begin(), bnapcm.end(),
              [](const auto& a, const auto& b) { return a.percent > b.percent; }
    );
}

SDL_Texture* loadSVG(const std::string& filename, SDL_Renderer* renderer) {
    NSVGimage* svg = nsvgParseFromFile(resource(filename).c_str(), "px", 96);
    if (!svg) {
        SDL_Log("SVG loading failed");
        return nullptr;
    }

    int width = static_cast<int>(svg->width);
    int height = static_cast<int>(svg->height);

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    std::vector<unsigned char> pixels(width * height * 4);

    nsvgRasterize(
        rast,
        svg,
        0, 0,
        1.0f,
        pixels.data(),
        width,
        height,
        width * 4
    );

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        pixels.data(),
        width * 4
    );

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg);

    return texture;
}

static bool openPreferences = false;

int main() {
    namespace fs = std::filesystem;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "SDL_Init failed", nullptr);
        return 1;
    }

    if (!TTF_Init()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "TTF_Init failed", nullptr);
        return 1;
    }

    int WIDTH = 1920, HEIGHT = 1080;
    SDL_Window *window = SDL_CreateWindow("Budget", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Failed to make window: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    TTF_Font* font = loadFont(18, renderer);
    TTF_Font* big_font = loadFont(20, renderer);

    if (!font) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        return 1;
    }

    if (!big_font) {
        SDL_Log("Failed to load beeg font: %s", SDL_GetError());
        return 1;
    }

    TTF_TextEngine* engine = TTF_CreateSurfaceTextEngine();
    if (!engine) {
        SDL_Log("TTF_CreateSurfaceTextEngine() failed: %s", SDL_GetError());
    }

    SDL_Texture* lastTex = loadSVG("last.svg", renderer);
    if (!lastTex) {
        return 1;
    }

    SDL_Texture* editTex = loadSVG("edit.svg", renderer);
    if (!editTex) {
        return 1;
    }

    display_mode mode = DEFAULT;
    display_mode last_mode = mode;

    constexpr int max_blocks = 32;
    Block blocks[max_blocks];

    int currYear;
    int currMonth;
    int selectedYear;
    int selectedMonth;
    string BDBDirectory;
    BDBTimes times {};
    int last_selectedYear;
    int last_selectedMonth;

    bool run = true;
    SDL_Event event;

    static const char* months[] = {
        "January", "February", "March", "April",
        "May", "June", "July", "August",
        "September", "October", "November", "December"
    };

    static SDL_Color monthColors[12] = {
        {163, 206, 241, 255}, // January
        {245, 183, 177, 255}, // February
        {169, 223, 191, 255}, // March
        {247, 220, 111, 255}, // April
        {125, 206, 160, 255}, // May
        {248, 196, 113, 255}, // June
        {236, 112, 99,  255}, // July
        {243, 156, 18,  255}, // August
        {133, 193, 233, 255}, // September
        {214, 137, 16,  255}, // October
        {165, 105, 189, 255}, // November
        {84,  153, 199, 255}  // December
    };

    auto now = chrono::system_clock::now();

    auto today = floor<chrono::days>(now);
    chrono::year_month_day ymd = chrono::year_month_day{today};

    const char* userHome = getUserHome();
    #ifdef __APPLE__
    string BudgetDirectory = fmt("%s/Library/Application Support/Budget", userHome);
    #else
    string BudgetDirectory = fmt("%s/.budget", userHome);
    #endif

    BDBDirectory = path(BudgetDirectory, "BDB");

    fs::create_directories(BDBDirectory);

    string tmp = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), static_cast<int>(ymd.year()), static_cast<unsigned>(ymd.month()));
    const char* filename = tmp.c_str();

    currMonth = static_cast<int>(static_cast<unsigned>(ymd.month())) - 1; // ymd.month() can't be cast directly to int
    currYear = static_cast<int>(ymd.year());

    ReadBlocksFromFile(blocks, max_blocks, filename);

    auto copyOldBlocksToCurrent = [&] {
        int buttonID = 0;
        SDL_MessageBoxButtonData buttonData[] = {
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Copy"}
        };
        SDL_MessageBoxData data {SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT,
            window, "Are you sure?",
            "You are about to copy the block names from the previous month to the current, this operation is not reversible, please make sure you want to do this",
            2, buttonData};
        SDL_ShowMessageBox(&data, &buttonID);
        if (buttonID == 0) {
            return;
        }
        int year = static_cast<int>(ymd.year());
        unsigned month = static_cast<unsigned>(ymd.month()) - 1;
        if (mode == VIEW && (selectedMonth != currMonth || selectedYear != currYear)) {
            year = selectedYear;
            month = selectedMonth; // selectedMonth is one less, no need to subtract
        }
        if (month < 1) { // goto last years last month if current is january
            month = 12;
            year -= 1;
        }
        string filepath = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), year, month);
        printf("%s\n", filepath.c_str());
        Block b[max_blocks];
        ReadBlocksFromFile(b, max_blocks, filepath.c_str());

        for (int i = 0; i < max_blocks; i++) {
            blocks[i] = b[i];
        }
    };

    times = getAllTimes(BDBDirectory);
    selectedYear = currYear;
    selectedMonth = currMonth;
    last_selectedYear = selectedYear;
    last_selectedMonth = selectedMonth;
    int currYearIndex = 0;
    for (int i = 0; i < 32; i++) {
        auto &year = times.years[i];
        if (year.year == currYear) {
            currYearIndex = i;
            break;
        }
    }

    std::vector<Menu> menus = {
        {"Budget", {
                {"Preferences", [&]{ openPreferences = !openPreferences; }, nullptr, {COMMAND, ','}}
            }, true
        },
        {"File", {
                {"Reset Month", [&] {
                    int buttonID = 0;
                    SDL_MessageBoxButtonData buttonData[] = {
                        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
                        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Reset"}
                    };
                    SDL_MessageBoxData data {SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT,
                        window, "Are you sure?",
                        "You are about to reset this month, it is not reversible",
                        2, buttonData};
                    SDL_ShowMessageBox(&data, &buttonID);
                    if (buttonID == 0) {
                        return;
                    }
                    for (auto & block : blocks) {
                        if (block.blank) continue;
                        DeleteBlock(block);
                    }
                }, nullptr, {SHIFT | COMMAND, 'r'}},
                {"Delete Month", [&] {
                    if (mode != VIEW || (selectedYear == currYear && selectedMonth == currMonth)) {
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Remember", "You can't delete this month, use ⇧+⌘+R instead to reset", window);
                        return;
                    }
                    string file = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), selectedYear, selectedMonth + 1);
                    int buttonID = 0;
                    SDL_MessageBoxButtonData buttonData[] = {
                        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
                        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Delete"}
                    };
                    SDL_MessageBoxData data {SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT,
                        window, "Are you sure?",
                        "You are about to delete this month, it will be permanently deleted.",
                        2, buttonData};
                    SDL_ShowMessageBox(&data, &buttonID);
                    if (buttonID == 1) { // confirm
                        fs::remove(file.c_str());
                        times = getAllTimes(BDBDirectory);
                        selectedMonth += 1;
                        if (selectedMonth >= 12) {
                            selectedMonth = 0;
                            selectedYear += 1;
                        }
                        last_selectedYear = selectedYear;
                        last_selectedMonth = selectedMonth;
                    }
                }, nullptr, {SHIFT | COMMAND, '\b'}},
                {"Carry over block structure", [&] {
                    copyOldBlocksToCurrent();
                }, nullptr, {SHIFT | COMMAND, 'b'}}
            }
        }
    };
    CreatePlatformMenu(menus);

    int selectedBlock = 0;
    float graphMultiplier = 0;

    string settingsFile = path(BudgetDirectory, "settings.json");
    Preferences prefs = loadSettings(settingsFile);

    int fps = 60;
    auto delay = static_cast<unsigned int>(1000.0 / fps);

    static constexpr float SCROLL_FRICTION = 0.9f;

    ScrollBar mainScrollbar {static_cast<float>(WIDTH) - 20, 0, 0, prefs.scrollSensitivity, prefs.scrollInversed, SCROLL_FRICTION};
    ScrollBar pieScrollbar {0, 0, 0, 0, false, 0};

    bool last_left = false;

    Block* expanded_block = nullptr;
    Block* delete_block = nullptr;
    Item* delete_item = nullptr;

    bool create_item = false;
    bool create_block = false;
    bool edit_item = false;
    int editing_item_index = -1;

    Textbox name{0, 0, 0, 0, font}, cost{0, 0, 0, 0, font};

    int blockCount = 0, totalContentHeight = 0, maxScrollY = 0;

    int expand[max_blocks];

    SDL_StartTextInput(window);
    while (run) {
        if (openPreferences) {
            SDL_SetWindowFocusable(window, false); // "disables" the window

            preferences(prefs, settingsFile, openPreferences, WIDTH, HEIGHT); // calls to open the preferences window

            SDL_SetWindowFocusable(window, true);  // "enables" the window
            SDL_RaiseWindow(window); // the window gets defocused, so focus it

            openPreferences = false;
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    run = false;
                    break;
                }
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                    if (event.window.windowID == SDL_GetWindowID(window)) run = false;
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: {
                    WIDTH = event.window.data1;
                    HEIGHT = event.window.data2;
                    break;
                }
                default: {
                    break;
                }
            }
            if (create_block && name.getInit()) {
                name.handleEvent(event);
            }
            if (create_item && name.getInit() && cost.getInit()) {
                name.handleEvent(event);
                cost.handleEvent(event);
            }
            if (mode != PIE && mode != GRAPH) {
                mainScrollbar.handleEvent(event);
            } else {
                pieScrollbar.handleEvent(event);
            }
        }
        if (mode != PIE && mode != GRAPH) {
            mainScrollbar.tick();
        } else {
            pieScrollbar.tick();
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        float mouse_x, mouse_y;
        SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        bool left = (buttons & SDL_BUTTON_LEFT) == 1;

        blockCount = 0;
        for (Block &block : blocks) {
            if (!block.blank) blockCount++;
        }

        totalContentHeight = blockCount * 110;
        maxScrollY = max(0, totalContentHeight - HEIGHT + 80 + (mode == VIEW) * 70);

        if (!create_block && !create_item && !delete_block && !delete_item && (mode == DEFAULT || mode == VIEW)) {
            bool blocks_exist = false;
            if (expanded_block == nullptr) {
                int y = 10 - static_cast<int>(mainScrollbar.scrollY);
                if (mode == VIEW) {
                    y += 70;
                }
                for (Block &block : blocks) {
                    if (block.blank) continue;
                    blocks_exist = true;

                    if (y + 100 < 0) {
                        y += 110;
                        continue;
                    }

                    if (y > HEIGHT - 40) {
                        break;
                    }

                    DrawRoundedRect(renderer, 30, 30, 30, 10, y, WIDTH - 50, 100, 10);
                    renderText(font, renderer, block.name, white, 20, y + 10);

                    renderText(font, renderer, fmt("%dkr", CalculateTotalCostOfBlock(block)).c_str(), white, 20, y + 67);

                    SDL_FRect button;
                    button.x = static_cast<float>(WIDTH - 75);
                    button.y = static_cast<float>(y + 5);
                    button.w = 30.0f;
                    button.h = 30.0f;

                    if (pointInRect(button, mouse_x, mouse_y)) {
                        int base = 40;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                block.expanded = !block.expanded;
                                last_left = false;
                            }
                        }
                        SDL_SetRenderDrawColor(renderer, base, base, base, 255);
                        DrawRoundedRect(renderer, base, base, base, &button, 10);
                    }

                    if (block.expanded) {
                        if (expanded_block == nullptr) {
                            expanded_block = &block;
                        } else if (expanded_block == &block) {
                        } else {
                            block.expanded = false;
                        }
                    } else {
                        if (expanded_block == &block) {
                            expanded_block = nullptr;
                        }
                    }

                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    if (block.expanded) {
                        renderText(big_font, renderer, "▼", white, WIDTH - 68, y + 7);
                    } else {
                        renderText(big_font, renderer, "◀", white, WIDTH - 68, y + 7);
                    }

                    button.x = static_cast<float>(WIDTH - 110);

                    if (pointInRect(button, mouse_x, mouse_y)) {
                        int base = 40;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                delete_block = &block;
                                last_left = false;
                            }
                        }
                        SDL_SetRenderDrawColor(renderer, base, base, base, 255);
                        DrawRoundedRect(renderer, base, base, base, &button, 10);
                    }

                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    DrawThickLineHV(renderer, WIDTH - 105, y + 20, WIDTH - 85, y + 20, 3);

                    y += 110;
                }

                DrawScrollGradient(renderer, WIDTH, 0, HEIGHT - 90, 60, {20, 20, 20});
                SDL_FRect close;
                close.x = 0.0f;
                close.y = static_cast<float>(HEIGHT - 30);
                close.w = static_cast<float>(WIDTH);
                close.h = 30.0f;
                SDL_RenderFillRect(renderer, &close);

                SDL_FRect button;
                button.x = static_cast<float>(WIDTH - 35);
                button.y = static_cast<float>(10);
                button.w = 30.0f;
                button.h = 30.0f;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            create_block = true;
                            if (name.getInit()) name.setText("");
                            name.setActive(true);
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                DrawThickLineHV(renderer, WIDTH - 20, 15, WIDTH - 20, 35, 3);
                DrawThickLineHV(renderer, WIDTH - 30, 25, WIDTH - 10, 25, 3);

                if (mode == VIEW) {
                    int x = 10;
                    for (auto & i : times.years) {
                        if (i.year == 0) continue;
                        y = 10 - static_cast<int>(mainScrollbar.scrollY);

                        string text = fmt("%d", i.year);

                        SDL_FRect year;
                        year.x = static_cast<float>(x);
                        year.y = static_cast<float>(y);
                        year.w = static_cast<float>(GetTextWidth(font, text.c_str())) + 10;
                        year.h = 30;

                        int base = 30 + (selectedYear == i.year) * 10;
                        if (pointInRect(year, mouse_x, mouse_y)) {
                            base = 40;
                            if (last_left) {
                                base += 10;
                                if (!left) {
                                    selectedYear = i.year;
                                    last_left = false;
                                }
                            }
                        }

                        DrawRoundedRect(renderer, base, base, base, &year, 10);
                        renderTextCenteredY(font, renderer, text.c_str(), white, x + 5, y + 15);
                        y += 35;

                        if (selectedYear == i.year) {
                            int month_x = 10;
                            for (int j = 0; j < 12; j++) {
                                if (i.months[j]) {
                                    string month_text = fmt("%s", months[j]);

                                    SDL_FRect month;
                                    month.x = static_cast<float>(month_x);
                                    month.y = static_cast<float>(y);
                                    month.w = static_cast<float>(GetTextWidth(font, month_text.c_str())) + 10;
                                    month.h = 30;

                                    base = 30 + (selectedMonth == j) * 10;
                                    if (pointInRect(month, mouse_x, mouse_y)) {
                                        base = 40;
                                        if (last_left) {
                                            base += 10;
                                            if (!left) {
                                                selectedMonth = j;
                                                last_left = false;
                                            }
                                        }
                                    }

                                    DrawRoundedRect(renderer, base, base, base, &month, 10);
                                    renderTextCenteredY(font, renderer, month_text.c_str(), white, month_x + 5, y + 15);

                                    month_x += GetTextWidth(font, month_text.c_str()) + 15;
                                }
                            }
                        }

                        x += static_cast<int>(year.w) + 5;
                    }
                }
            } else {
                int y = 10;
                Block &block = *expanded_block;
                blocks_exist = true;

                DrawRoundedRect(renderer, 30, 30, 30, 10, y, WIDTH - 20, HEIGHT - 20, 10);
                renderText(font, renderer, block.name, white, 20, y + 10);

                renderText(font, renderer, fmt("Total: %dkr", CalculateTotalCostOfBlock(block)).c_str(), white, 20, y + HEIGHT - 53);

                SDL_FRect button;
                button.x = static_cast<float>(WIDTH - 45);
                button.y = static_cast<float>(y + 5);
                button.w = 30.0f;
                button.h = 30.0f;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            block.expanded = false;
                            expanded_block = nullptr;
                            last_left = false;
                        }
                    }
                    SDL_SetRenderDrawColor(renderer, base, base, base, 255);
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                if (block.expanded) {
                    renderText(big_font, renderer, "▼", white, WIDTH - 38, y + 7);
                } else {
                    renderText(big_font, renderer, "◀", white, WIDTH - 38, y + 7);
                }

                button.x -= 35;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            create_item = true;
                            if (name.getInit()) name.setText("");
                            if (cost.getInit()) cost.setText("");
                            name.setActive(true);
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                DrawThickLineHV(renderer, WIDTH - 65, y + 10, WIDTH - 65, y + 30, 3);
                DrawThickLineHV(renderer, WIDTH - 75, y + 20, WIDTH - 55, y + 20, 3);

                // Start rendering the items
                int x = 20;
                y = 50;
                for (int i = 0; i < 128; i++) {
                    Item &item = block.items[i];
                    if (item.blank) continue;

                    int name_w = GetTextWidth(font, item.name);
                    int cost_w = GetTextWidth(font, fmt("%dkr", item.cost).c_str());
                    int w = max(max(cost_w, name_w + 50), 100);

                    if (x + w > WIDTH - 20) {
                        x = 20;
                        y += 110;
                    }

                    DrawRoundedRect(renderer, 40, 40, 40, x, y, w, 100, 10);
                    renderText(font, renderer, item.name, white, x + 10, y + 10);

                    renderText(font, renderer, fmt("%dkr", item.cost).c_str(), white, x + 10, y + 70);

                    button.x = static_cast<float>(x + w - 35);
                    button.y = static_cast<float>(y + 5);

                    if (pointInRect(button, mouse_x, mouse_y)) {
                        int base = 50;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                delete_item = &item;
                                last_left = false;
                            }
                        }
                        SDL_SetRenderDrawColor(renderer, base, base, base, 255);
                        DrawRoundedRect(renderer, base, base, base, &button, 10);
                    }

                    button.y += 35;

                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    DrawThickLineHV(renderer, x + w - 30, y + 20, x + w - 10, y + 20, 3);

                    if (pointInRect(button, mouse_x, mouse_y)) {
                        int base = 50;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                create_item = true;
                                edit_item = true;
                                editing_item_index = i;
                                if (name.getInit()) name.setText(item.name);
                                if (cost.getInit()) cost.setText(fmt("%d", item.cost).c_str());
                                last_left = false;
                            }
                        }
                        SDL_SetRenderDrawColor(renderer, base, base, base, 255);
                        DrawRoundedRect(renderer, base, base, base, &button, 10);
                    }

                    button.w -= 5;
                    button.h -= 5;
                    button.x += 2.5;
                    button.y += 2.5;

                    SDL_RenderTexture(renderer, editTex, nullptr, &button);

                    x += w + 10;
                }
            }

            // bookmark: just a bookmark
            if (blocks_exist && expanded_block == nullptr) {
                renderText(font, renderer, fmt("Total: %dkr | Date: %d-%02d", CalculateTotalCostOfBlocks(blocks, max_blocks), selectedYear, selectedMonth + 1).c_str(), white, 10, HEIGHT - 30);

                SDL_FRect button;
                button.x = static_cast<float>(WIDTH - 35);
                button.y = static_cast<float>(45);
                button.w = 30.0f;
                button.h = 30.0f;

                int base = 20;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            last_mode = mode;
                            mode = PIE;
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                DrawCircle(
                    renderer,
                    static_cast<int>(button.x) + 15,
                    static_cast<int>(button.y) + 15,
                    10
                );

                DrawPieSlice(renderer, nullptr,
                    static_cast<int>(button.x) + 15,
                    static_cast<int>(button.y) + 15, 11, 0.25,
                    {
                        static_cast<float>(base) / 255.0f,
                        static_cast<float>(base) / 255.0f,
                        static_cast<float>(base) / 255.0f,
                        1.0f
                    }
                );

                button.y += 35;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            if (mode == VIEW) {
                                if (selectedYear != currYear || selectedMonth != currMonth) {
                                    SDL_ShowSimpleMessageBox(0, "Hey!", "You need to switch to the current month before exiting!", window);
                                    continue;
                                }
                                mode = DEFAULT;
                                last_mode = mode;
                                string file = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), selectedYear, selectedMonth + 1);
                                ReadBlocksFromFile(blocks, max_blocks, filename);
                            } else {
                                last_mode = mode;
                                mode = VIEW;
                                WriteBlocksToFile(blocks, max_blocks, filename);
                                selectedYear = currYear;
                                selectedMonth = currMonth;
                            }
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                int cx = static_cast<int>(button.x) + 15;
                int cy = static_cast<int>(button.y) + 15;

                double start = -M_PI / 2;
                constexpr double gap = 0.02;
                constexpr double by = 1.0f / 12.0f - gap;
                start -= by * M_PI;
                for (int i = 0; i < 12; i++) {
                    start = DrawPieSlice(renderer, nullptr, cx, cy, 10, by, {1.0f, 1.0f, 1.0f, 1.0f}, start);
                    start += gap * 2 * M_PI;
                }

                button.y += 35;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            last_mode = mode;
                            mode = GRAPH;
                            graphMultiplier = 0;
                            selectedBlock = 0;
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_FRect rect;
                rect.w = 5;
                rect.h = 15;
                rect.x = button.x + 5;
                rect.y = button.y + button.h - rect.h - 2;
                SDL_RenderFillRect(renderer, &rect);
                rect.x += 7;
                rect.h += 5;
                rect.y = button.y + button.h - rect.h - 2;
                SDL_RenderFillRect(renderer, &rect);
                rect.x += 7;
                rect.h -= 7;
                rect.y = button.y + button.h - rect.h - 2;
                SDL_RenderFillRect(renderer, &rect);

                if (maxScrollY > 0) {
                    mainScrollbar.x = static_cast<float>(WIDTH) - 20;
                    mainScrollbar.y = button.y + button.h + 10;
                    mainScrollbar.height = static_cast<float>(HEIGHT) - mainScrollbar.y - 100;
                    mainScrollbar.draw(renderer, static_cast<float>(maxScrollY), static_cast<float>(totalContentHeight), static_cast<float>(HEIGHT) - 80);
                }

            } else if (!blocks_exist) {
                renderText(font, renderer, "Press the + icon to add a new block:", white, WIDTH - 435, 12);
                SDL_FRect button{};
                button.x = static_cast<float>(WIDTH - 35);
                button.y = static_cast<float>(45);
                button.w = 30.0f;
                button.h = 30.0f;
                if (pointInRect(button, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            copyOldBlocksToCurrent();
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                SDL_RenderTexture(renderer, lastTex, nullptr, &button);
            }
        } else {
            int x = WIDTH / 8;
            int y = HEIGHT / 8;
            int w = WIDTH - x * 2;
            int h = HEIGHT - y * 2;
            DrawRoundedRect(renderer, 30, 30, 30, x, y, w, h, 10);

            if (mode != PIE && mode != GRAPH) {
                if (create_block || create_item) {
                    SDL_FRect cancel, create;

                    cancel.x = static_cast<float>(x + 10);
                    cancel.y = static_cast<float>(y + h - 40);
                    cancel.w = 70;
                    cancel.h = 30;

                    int base = 235;
                    if (pointInRect(cancel, mouse_x, mouse_y)) {
                        base += 10;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                create_block = false;
                                create_item = false;
                                last_left = false;
                            }
                        }
                    }

                    DrawRoundedRect(renderer, base, base, base, &cancel, 10);
                    renderTextCentered(font, renderer, "Cancel", black, cancel.x + 35, cancel.y + 15);

                    create.x = static_cast<float>(x + w - 80);
                    create.y = static_cast<float>(y + h - 40);
                    create.w = 70;
                    create.h = 30;

                    bool create_pressed = false;
                    base = 235;
                    if (pointInRect(create, mouse_x, mouse_y)) {
                        base += 10;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                create_pressed = true;
                            }
                        }
                    }

                    DrawRoundedRect(renderer, base, base, base, &create, 10);
                    if (edit_item)
                        renderTextCentered(font, renderer, "Save", black, create.x + 35, create.y + 15);
                    else
                        renderTextCentered(font, renderer, "Create", black, create.x + 35, create.y + 15);

                    if (create_block) {
                        renderText(big_font, renderer, "Create new block", white, x + 10, y + 10);

                        renderText(font, renderer, "Name:", white, x + 10, y + 50);
                        name.init(x + 10, y + 75, w - 20, 30, font);

                        name.draw(renderer);

                        if (create_pressed) {
                            create_block = false;
                            int firstEmpty = FindFirstEmptyBlock(blocks, max_blocks);
                            if (firstEmpty == -1) {
                                SDL_ShowSimpleMessageBox(0, "Uh oh", "Seems like I can't add more blocks :(", window);
                                continue;
                            }
                            blocks[firstEmpty] = CreateBlock(name.getText());
                        }
                    }
                    if (create_item && !edit_item) {
                        renderText(big_font, renderer, "Create new item", white, x + 10, y + 10);

                        name.init(x + 10, y + 75, w - 20, 30, font);

                        renderText(font, renderer, "Name:", white, x + 10, y + 50);
                        name.draw(renderer);

                        cost.init(x + 10, y + 150, w - 20, 30, font);

                        renderText(font, renderer, "Cost:", white, x + 10, y + 125);
                        cost.draw(renderer);

                        if (create_pressed) {
                            create_item = false;
                            if (!expanded_block) {
                                SDL_Log("expanded_block is nullptr");
                                break;
                            }
                            int firstEmpty = FindFirstEmptyItem(*expanded_block);
                            if (firstEmpty == -1) {
                                SDL_ShowSimpleMessageBox(0, "Uh oh", "The block is full, no more items can be added", window);
                                continue;
                            }
                            int int_cost = parseInt(cost.getText());
                            expanded_block->items[firstEmpty] = CreateItem(name.getText(), int_cost);
                        }
                    } else if (create_item && edit_item) {
                        renderText(big_font, renderer, "Edit item", white, x + 10, y + 10);

                        name.init(x + 10, y + 75, w - 20, 30, font);

                        renderText(font, renderer, "Name:", white, x + 10, y + 50);
                        name.draw(renderer);

                        cost.init(x + 10, y + 150, w - 20, 30, font);

                        renderText(font, renderer, "Cost:", white, x + 10, y + 125);
                        cost.draw(renderer);

                        if (create_pressed) {
                            create_item = false;
                            edit_item = false;
                            if (!expanded_block) {
                                SDL_Log("expanded_block is nullptr");
                                break;
                            }
                            int firstEmpty = editing_item_index;
                            int int_cost = parseInt(cost.getText());
                            if (int_cost < 0) {
                                SDL_ShowSimpleMessageBox(0, "Hey", "The cost has to be a positive number", window);
                                continue;
                            }
                            expanded_block->items[firstEmpty] = CreateItem(name.getText(), int_cost);
                        }
                    }
                }
                if (delete_block) {
                    Block &block = *delete_block;

                    renderText(big_font, renderer, "Are you sure?", white, x + 10, y + 10);

                    renderText(font, renderer, fmt("You're about to delete a block called: \"%s\"", block.name).c_str(), white, x + 10, y + 50);

                    SDL_FRect cancel, delete_;

                    cancel.x = static_cast<float>(x + 10);
                    cancel.y = static_cast<float>(y + h - 40);
                    cancel.w = 70;
                    cancel.h = 30;

                    int base = 235;
                    if (pointInRect(cancel, mouse_x, mouse_y)) {
                        base += 10;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                delete_block = nullptr;
                                last_left = false;
                            }
                        }
                    }

                    DrawRoundedRect(renderer, base, base, base, &cancel, 10);
                    renderTextCentered(font, renderer, "Cancel", black, cancel.x + 35, cancel.y + 15);

                    delete_.x = static_cast<float>(x + w - 80);
                    delete_.y = static_cast<float>(y + h - 40);
                    delete_.w = 70;
                    delete_.h = 30;

                    // ReSharper disable CppDFANullDereference

                    base = 235;
                    if (pointInRect(delete_, mouse_x, mouse_y)) {
                        base += 10;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                DeleteBlock(block);
                                delete_block = nullptr;
                            }
                        }
                    }

                    DrawRoundedRect(renderer, base, base, base, &delete_, 10);
                    renderTextCentered(font, renderer, "Delete", black, delete_.x + 35, delete_.y + 15);
                }

                if (delete_item) {
                    Item &item = *delete_item;

                    renderText(big_font, renderer, "Are you sure?", white, x + 10, y + 10);

                    renderText(font, renderer, fmt("You're about to delete an item called: \"%s\"", item.name).c_str(), white, x + 10, y + 50);

                    SDL_FRect cancel, delete_;

                    cancel.x = static_cast<float>(x + 10);
                    cancel.y = static_cast<float>(y + h - 40);
                    cancel.w = 70;
                    cancel.h = 30;

                    int base = 235;
                    if (pointInRect(cancel, mouse_x, mouse_y)) {
                        base += 10;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                delete_item = nullptr;
                                last_left = false;
                            }
                        }
                    }

                    DrawRoundedRect(renderer, base, base, base, &cancel, 10);
                    renderTextCentered(font, renderer, "Cancel", black, cancel.x + 35, cancel.y + 15);

                    delete_.x = static_cast<float>(x + w - 80);
                    delete_.y = static_cast<float>(y + h - 40);
                    delete_.w = 70;
                    delete_.h = 30;

                    base = 235;
                    if (pointInRect(delete_, mouse_x, mouse_y)) {
                        base += 10;
                        if (last_left) {
                            base += 10;
                            if (!left) {
                                DeleteItem(item);
                                delete_item = nullptr;
                            }
                        }
                    }

                    DrawRoundedRect(renderer, base, base, base, &delete_, 10);
                    renderTextCentered(font, renderer, "Delete", black, delete_.x + 35, delete_.y + 15);
                }
            } else if (mode == PIE) { // bookmark: pie start
                // Draw the pie chart
                double startAngle = -M_PI / 2.0;
                int total = CalculateTotalCostOfBlocks(blocks, max_blocks);

                int radius = min(w, h) / 2 - 50;
                int cx = x + w - radius - 25;
                int cy = HEIGHT / 2;

                vector<BlockNameAndPercentColorMix> blocksAndColors = {};

                for (Block &block : blocks) {
                    if (block.blank) continue;
                    int block_cost = CalculateTotalCostOfBlock(block);
                    if (block_cost <= 0) continue;
                    double percent = static_cast<double>(block_cost) / static_cast<double>(total);
                    SDL_Color c = BlockToColor(block);
                    blocksAndColors.push_back(BlockNameAndPercentColorMix {block.name, percent, c, CalculateTotalCostOfBlock(block)});
                }

                SortBlockNameAndPercentColorMixByPercent(blocksAndColors);

                int hoveredOn = -1;
                for (int i = 0; i < blocksAndColors.size(); i++) {
                    BlockNameAndPercentColorMix &block = blocksAndColors[i];
                    double percent = block.percent;
                    SDL_Color c = block.color;
                    vector<SDL_Vertex> vb;

                    startAngle = DrawPieSlice(
                        renderer,
                        &vb,
                        cx,
                        cy,
                        radius + expand[i] / 2,
                        percent,
                        SDL_FColor {
                            static_cast<float>(c.r) / 255.0f,
                            static_cast<float>(c.g) / 255.0f,
                            static_cast<float>(c.b) / 255.0f,
                            static_cast<float>(c.a) / 255.0f
                        },
                        startAngle
                    );

                    if (PointInVB(mouse_x, mouse_y, vb)) {
                        hoveredOn = i;
                        expand[i] = min(expand[i] + 1, 20);
                    } else {
                        expand[i] = max(expand[i] - 1, 0);
                    }
                }

                // Draw the button to get out
                SDL_FRect button;
                button.x = static_cast<float>(x + w - 40);
                button.y = static_cast<float>(y + 10);
                button.w = 30.0f;
                button.h = 30.0f;

                if (pointInRect(button, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            mode = last_mode;
                            last_mode = mode;
                        }
                    }
                    SDL_SetRenderDrawColor(renderer, base, base, base, 255);
                    DrawRoundedRect(renderer, base, base, base, &button, 10);
                }

                renderText(big_font, renderer, "◀", white, static_cast<int>(button.x) + 7, static_cast<int>(button.y) + 2);

                // And finally, draw what the different colors mean
                totalContentHeight = 50 * static_cast<int>(blocksAndColors.size());
                maxScrollY = max(0, totalContentHeight - h + 40);
                int colorX = x + (totalContentHeight > y + h - 10 ? 30 : 10);
                int colorY = y + 10 - static_cast<int>(pieScrollbar.scrollY);
                bool drawn_p = false;
                for (int i = 0; i < blocksAndColors.size(); i++) {
                    BlockNameAndPercentColorMix &blockAndPercent = blocksAndColors[i];
                    const char* blockName = blockAndPercent.block_name;
                    SDL_Color color = blockAndPercent.color;
                    int s = 40 + expand[i] / 2;
                    if (expand[i] >= 10 && !drawn_p) {
                        drawn_p = true;
                        double percent = blockAndPercent.percent;
                        SDL_Color colour = {255, 255, 255, static_cast<Uint8>(255.0f * (static_cast<float>(expand[i]) / 20.0f))};
                        renderText(big_font, renderer, fmt("%s"    , blockName)       .c_str(), colour, static_cast<int>(mouse_x) + 10, static_cast<int>(mouse_y) + 10);
                        renderText(big_font, renderer, fmt("%.1f%%", percent * 100)   .c_str(), colour, static_cast<int>(mouse_x) + 10, static_cast<int>(mouse_y) + 30);
                        renderText(big_font, renderer, fmt("%dkr"  , blockAndPercent.kr).c_str(), colour, static_cast<int>(mouse_x) + 10, static_cast<int>(mouse_y) + 50);
                    }
                    if (i == hoveredOn) {
                        if (colorY < y + 15) {
                            int sub = 0;
                            while (colorY + sub < y + 15) {
                                sub += 1;
                            }
                            pieScrollbar.scrollYVelocity = static_cast<float>(sub) * -0.15f;
                        } else if (colorY + s + 10 > y + h - 30) {
                            int add = 0;
                            while (colorY + s + 10 - add > y + h - 30) {
                                add += 1;
                            }
                            pieScrollbar.scrollYVelocity = static_cast<float>(add) * 0.15f;
                        }
                    }
                    if (colorY + 10 <= y + h - 10) {
                        DrawRoundedRect(renderer, color.r, color.g, color.b, colorX, colorY, s, s, 10);
                        renderTextCenteredY(font, renderer, fmt(" = %s", blockName).c_str(), white, colorX + s, colorY + s / 2);
                    }
                    colorY += s + 10;
                }
                if (colorX - x == 30) { // check if we need scrollbar
                    DrawScrollGradient(renderer, w, x + 10, y + h - 30, 15, { 30, 30, 30, 255 });
                    DrawScrollGradient(renderer, w, x + 10, y, 15, { 30, 30, 30, 255 }, true);
                    SDL_FRect rect;
                    rect.x = static_cast<float>(x) + 10;
                    rect.y = static_cast<float>(y + h - 15);
                    rect.w = static_cast<float>(w);
                    rect.h = 15.0f;
                    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
                    SDL_RenderFillRect(renderer, &rect);
                    rect.y += 15;
                    rect.h += 15;
                    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
                    SDL_RenderFillRect(renderer, &rect);
                    rect.h = static_cast<float>(y);
                    rect.y = 0;
                    rect.x = 0;
                    rect.w = static_cast<float>(w);
                    SDL_RenderFillRect(renderer, &rect);

                    pieScrollbar.x = static_cast<float>(x) + 15;
                    pieScrollbar.y = static_cast<float>(y) + 15;
                    pieScrollbar.height = static_cast<float>(h) - 30;
                    pieScrollbar.scrollSensitivity = prefs.scrollSensitivity;
                    pieScrollbar.scrollInversed = prefs.scrollInversed;
                    pieScrollbar.scrollFriction = SCROLL_FRICTION;
                    pieScrollbar.draw(renderer, static_cast<float>(maxScrollY), static_cast<float>(totalContentHeight), static_cast<float>(h) - 30);
                }
            } else { // bookmark: graph start
                float scale = 0.75f;
                graphMultiplier += (100 - graphMultiplier) / 20;
                float graphMultiplierScaled = graphMultiplier / 100.0f;

                int t = y + 50;
                int b = y + h - 50;
                int l = x + 50;
                int r = x + w - 50;

                float cx = static_cast<float>(l + r) * 0.5f;
                float cy = static_cast<float>(t + b) * 0.5f;

                l = static_cast<int>(cx + (static_cast<float>(l) - cx) * scale);
                r = static_cast<int>(cx + (static_cast<float>(r) - cx) * scale);
                t = static_cast<int>(cy + (static_cast<float>(t) - cy) * scale);
                b = static_cast<int>(cy + (static_cast<float>(b) - cy) * scale);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                DrawThickLineHV(renderer, l, b, l, t, 5, true);
                DrawThickLineHV(renderer, l, b, r, b, 5, true);

                renderTextCentered(font, renderer, "kr", white, static_cast<float>(l) - 25, lerpf(static_cast<float>(t), static_cast<float>(b), 0.5f));

                renderTextCentered(big_font, renderer, blocks[selectedBlock].name, white, cx, static_cast<float>(y) + 25);

                int maxCost = 0;
                int existingMonths = 0;
                for (int i = 0; i < 12; i++) {
                    if (!times.years[currYearIndex].months[i]) continue;
                    existingMonths++;
                    string file = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), currYear, i + 1);
                    Block monthBlocks[max_blocks];
                    ReadBlocksFromFile(monthBlocks, max_blocks, file.c_str());

                    Block monthBlock;
                    if (Block* block = GetBlockByName(monthBlocks, max_blocks, blocks[selectedBlock].name)) {
                        monthBlock = *block;
                    } else {
                        continue;
                    }

                    int blockCost = CalculateTotalCostOfBlock(monthBlock);
                    blockCost += 50;
                    if (blockCost > maxCost) {
                        maxCost = blockCost;
                    }
                }

                SDL_FRect rect;
                rect.x = static_cast<float>(l) + 10.0f;
                rect.w = (static_cast<float>(r) - static_cast<float>(l) - 20.0f - 55.0f) / 12.0f;

                for (int i = 0; i < 12; i++) {
                    if (!times.years[currYearIndex].months[i]) continue;
                    string file = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), currYear, i + 1);
                    Block monthBlocks[max_blocks];
                    ReadBlocksFromFile(monthBlocks, max_blocks, file.c_str());

                    Block monthBlock;
                    if (Block* block = GetBlockByName(monthBlocks, max_blocks, blocks[selectedBlock].name)) {
                        monthBlock = *block;
                    } else {
                        continue;
                    }

                    int totalMonthCost = CalculateTotalCostOfBlock(monthBlock);

                    float height = static_cast<float>(totalMonthCost) / static_cast<float>(maxCost);
                    height *= static_cast<float>(b - t) * graphMultiplierScaled;
                    rect.h = height;
                    rect.y = static_cast<float>(b) - height;
                    SDL_SetRenderDrawColor(renderer, monthColors[i].r, monthColors[i].g, monthColors[i].b, 255);
                    SDL_RenderFillRect(renderer, &rect);
                    renderTextCentered(font, renderer, fmt("%ikr", totalMonthCost).c_str(), white, rect.x + rect.w / 2.0f, rect.y - 20);
                    rect.x += rect.w + 5;
                }

                rect.x = static_cast<float>(x) + 10;
                rect.y = static_cast<float>(y) + 10;
                rect.w = 40;
                rect.h = 40;

                for (int i = 0; i < 12; i++) {
                    if (!times.years[currYearIndex].months[i]) continue;
                    SDL_Color color = monthColors[i];
                    DrawRoundedRect(renderer, color.r, color.g, color.b, &rect, 10);

                    const char text[4] = {months[i][0], months[i][1], months[i][2], '\0'};
                    renderTextCentered(font, renderer, text, white, rect.x + rect.w / 2, rect.y + rect.h / 2);

                    rect.y += 45;
                    if (rect.y + 45 > static_cast<float>(y + h) - 50) {
                        rect.y = static_cast<float>(y) + 10;
                        rect.x += 45;
                    }
                }

                rect.x = static_cast<float>(x + w) - 40;
                rect.y = static_cast<float>(y) + 10;
                rect.w = 30;
                rect.h = 30;

                if (pointInRect(rect, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left) {
                            mode = last_mode;
                            last_mode = mode;
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &rect, 10);
                }

                renderText(big_font, renderer, "◀", white, static_cast<int>(rect.x) + 7, static_cast<int>(rect.y) + 2);

                rect.x = static_cast<float>(x) + 10;
                rect.y = static_cast<float>(y + h) - 40;

                if (pointInRect(rect, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left && selectedBlock > 0) {
                            selectedBlock--;
                            graphMultiplier = 0;
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &rect, 10);
                }

                if (selectedBlock == 0) {
                    renderText(big_font, renderer, "◁", white, static_cast<int>(rect.x) + 7, static_cast<int>(rect.y) + 2);
                } else {
                    renderText(big_font, renderer, "◀", white, static_cast<int>(rect.x) + 7, static_cast<int>(rect.y) + 2);
                }

                rect.x += static_cast<float>(w) - 50;

                if (pointInRect(rect, mouse_x, mouse_y)) {
                    int base = 40;
                    if (last_left) {
                        base += 10;
                        if (!left && selectedBlock < blockCount - 1) {
                            selectedBlock++;
                            graphMultiplier = 0;
                        }
                    }
                    DrawRoundedRect(renderer, base, base, base, &rect, 10);
                }

                if (selectedBlock == blockCount - 1) {
                    renderTextCentered(big_font, renderer, "▷", white, rect.x + 15, rect.y + 15);
                } else {
                    renderTextCentered(big_font, renderer, "▶", white, rect.x + 15, rect.y + 15);
                }
            }
        }

        SDL_RenderPresent(renderer);

        if (mode == VIEW) {
            if (selectedYear != last_selectedYear || selectedMonth != last_selectedMonth) {
                string file = fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), selectedYear, selectedMonth + 1);
                WriteBlocksToFile(blocks, max_blocks, fmt(path(BDBDirectory, "%d-%02d.bin").c_str(), last_selectedYear, last_selectedMonth + 1).c_str());
                ReadBlocksFromFile(blocks, max_blocks, file.c_str());
                last_selectedYear = selectedYear;
                last_selectedMonth = selectedMonth;
            }
        }

        last_left = left;

        SDL_Delay(delay);
    }

    SDL_DestroyTexture(editTex);
    SDL_DestroyTexture(lastTex);
    TTF_CloseFont(font);
    TTF_CloseFont(big_font);
    TTF_DestroySurfaceTextEngine(engine);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_StopTextInput(window);

    if (mode == DEFAULT) {
        if (!WriteBlocksToFile(blocks, max_blocks, filename)) SDL_Log("Could not save file");
    }

    SDL_Quit();
    TTF_Quit();
    return 0;
}
