//
// Created by Ayaan Banday on 2025-09-22.
//

#ifndef BUDGET_HELPERS_H
#define BUDGET_HELPERS_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <charconv>
#include <vector>
#include <string>

#include <cmath>

constexpr char PATH_SEPARATOR = '/';
const std::string ROOT = "/";

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

using namespace std;

static constexpr SDL_Color white = { 255, 255, 255, 255 };
static constexpr SDL_Color black = { 0,   0,   0,   255 };

inline void DrawFilledCircleQuarter(SDL_Renderer* renderer, int ccx, int ccy, int r, int corner)
{
    auto x = static_cast<float>(r);
    float y = 0;
    int err = 1 - r;
    auto cx = static_cast<float>(ccx), cy = static_cast<float>(ccy);

    while (x >= y)
    {
        // For each octant, we want only one quarter
        switch (corner)
        {
            case 0: // TL
                SDL_RenderLine(renderer, cx - x, cy - y, cx, cy - y);
                SDL_RenderLine(renderer, cx - y, cy - x, cx, cy - x);
                break;
            case 1: // TR
                SDL_RenderLine(renderer, cx, cy - y, cx + x, cy - y);
                SDL_RenderLine(renderer, cx, cy - x, cx + y, cy - x);
                break;
            case 2: // BR
                SDL_RenderLine(renderer, cx, cy + y, cx + x, cy + y);
                SDL_RenderLine(renderer, cx, cy + x, cx + y, cy + x);
                break;
            case 3: // BL
                SDL_RenderLine(renderer, cx - x, cy + y, cx, cy + y);
                SDL_RenderLine(renderer, cx - y, cy + x, cx, cy + x);
                break;
            default:;
        }

        y++;
        if (err < 0)
        {
            err += static_cast<int>(2 * y + 1);
        }
        else
        {
            x--;
            err += static_cast<int>(2 * (y - x + 1));
        }
    }
}

inline void DrawRoundedRect(SDL_Renderer* renderer, int red, int green, int blue, const SDL_FRect* rect, int r) {
    int x = static_cast<int>(rect->x);
    int y = static_cast<int>(rect->y);
    int w = static_cast<int>(rect->w);
    int h = static_cast<int>(rect->h);

    r = min(r, min(w/2, h/2));

    SDL_FRect horzRect;
    horzRect.x = static_cast<float>(x + r);
    horzRect.y = static_cast<float>(y);
    horzRect.w = static_cast<float>(w - 2*r);
    horzRect.h = static_cast<float>(h);

    SDL_FRect vertRect;
    vertRect.x = static_cast<float>(x);
    vertRect.y = static_cast<float>(y + r);
    vertRect.w = static_cast<float>(w);
    vertRect.h = static_cast<float>(h - 2*r);

    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_RenderFillRect(renderer, &horzRect);
    SDL_RenderFillRect(renderer, &vertRect);

    DrawFilledCircleQuarter(renderer, x + r,         y + r,         r, 0);
    DrawFilledCircleQuarter(renderer, x + w - r - 1, y + r,         r, 1);
    DrawFilledCircleQuarter(renderer, x + w - r - 1, y + h - r - 1, r, 2);
    DrawFilledCircleQuarter(renderer, x + r,         y + h - r - 1, r, 3);
}

inline void DrawRoundedRect(SDL_Renderer* renderer, int red, int green, int blue, int x, int y, int w, int h, int r) {
    r = min(r, min(w/2, h/2));

    SDL_FRect horzRect;
    horzRect.x = static_cast<float>(x + r);
    horzRect.y = static_cast<float>(y);
    horzRect.w = static_cast<float>(w - 2*r);
    horzRect.h = static_cast<float>(h);

    SDL_FRect vertRect;
    vertRect.x = static_cast<float>(x);
    vertRect.y = static_cast<float>(y + r);
    vertRect.w = static_cast<float>(w);
    vertRect.h = static_cast<float>(h - 2*r);

    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_RenderFillRect(renderer, &horzRect);
    SDL_RenderFillRect(renderer, &vertRect);

    DrawFilledCircleQuarter(renderer, x + r,         y + r,         r, 0);
    DrawFilledCircleQuarter(renderer, x + w - r - 1, y + r,         r, 1);
    DrawFilledCircleQuarter(renderer, x + w - r - 1, y + h - r - 1, r, 2);
    DrawFilledCircleQuarter(renderer, x + r,         y + h - r - 1, r, 3);
}

inline int renderText(TTF_Font* font, SDL_Renderer* renderer, const char* text, SDL_Color color, int x, int y) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface) return 0;
    int w = surface->w;

    int texW = surface->w;
    int texH = surface->h;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FRect dstRect{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(texW), static_cast<float>(texH) };
    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
    return w;
}

inline void renderTextCentered(TTF_Font* font, SDL_Renderer* renderer,
                               const char* text, SDL_Color color,
                               float centerX, float centerY) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, SDL_strlen(text), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { SDL_DestroySurface(surface); return; }

    float textW = 0, textH = 0;
    SDL_GetTextureSize(texture, &textW, &textH);

    SDL_FRect dstRect;
    dstRect.x = centerX - (textW * 0.5f);
    dstRect.y = centerY - (textH * 0.5f);
    dstRect.w = textW;
    dstRect.h = textH;

    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

inline void renderTextCenteredY(TTF_Font* font, SDL_Renderer* renderer,
                               const char* text, SDL_Color color,
                               int x, int centerY) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, SDL_strlen(text), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { SDL_DestroySurface(surface); return; }

    float textW = 0, textH = 0;
    SDL_GetTextureSize(texture, &textW, &textH);

    SDL_FRect dstRect;
    dstRect.x = static_cast<float>(x);
    dstRect.y = static_cast<float>(centerY) - (textH * 0.5f);
    dstRect.w = textW;
    dstRect.h = textH;

    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

// Draw a circle centered at (cx, cy) with radius r
inline void DrawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    int x = radius;
    int y = 0;
    int decision = 1 - x;

    const auto cx = static_cast<float>(centerX);
    const auto cy = static_cast<float>(centerY);

    while (y <= x) {
        // Draw horizontal lines instead of just pixels
        const auto fx = static_cast<float>(x);
        const auto fy = static_cast<float>(y);
        SDL_RenderLine(renderer, cx - fx, cy + fy, cx + fx, cy + fy);
        SDL_RenderLine(renderer, cx - fx, cy - fy, cx + fx, cy - fy);
        SDL_RenderLine(renderer, cx - fy, cy + fx, cx + fy, cy + fx);
        SDL_RenderLine(renderer, cx - fy, cy - fx, cx + fy, cy - fx);

        y++;
        if (decision <= 0) {
            decision += 2 * y + 1;
        } else {
            x--;
            decision += 2 * (y - x) + 1;
        }
    }
}

inline void DrawThickLineHV(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int thickness, bool rounded_edges=false) {
    SDL_FRect rect;

    if (x1 == x2) {
        // Vertical line
        rect.x = static_cast<float>(x1 - thickness / 2);
        rect.y = static_cast<float>(y1 < y2 ? y1 : y2);
        rect.w = static_cast<float>(thickness);
        rect.h = static_cast<float>(abs(y2 - y1));
    } else if (y1 == y2) {
        // Horizontal line
        rect.x = static_cast<float>(x1 < x2 ? x1 : x2);
        rect.y = static_cast<float>(y1 - thickness / 2);
        rect.w = static_cast<float>(abs(x2 - x1));
        rect.h = static_cast<float>(thickness);
    } else {
        // Not horizontal or vertical; do nothing
        return;
    }

    SDL_RenderFillRect(renderer, &rect);

    if (rounded_edges) {
        DrawCircle(renderer, x1, y1, thickness / 2 - 1);
        DrawCircle(renderer, x2, y2, thickness / 2 - 1);
    }
}

inline void GetRenderDrawColor(SDL_Renderer* renderer, SDL_FColor &color) {
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
}

inline void DrawThickLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float thickness) {
    // Compute the line vector
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt(dx*dx + dy*dy);

    if (length == 0) return; // avoid zero-length line

    // Unit perpendicular vector
    float ux = -dy / length;
    float uy = dx / length;

    // Half thickness offset
    float hx = ux * (thickness / 2.0f);
    float hy = uy * (thickness / 2.0f);

    // Define the 4 vertices of the quad
    SDL_Vertex vertices[4];

    SDL_FColor color;

    GetRenderDrawColor(renderer, color);

    vertices[0].position.x = x1 + hx; vertices[0].position.y = y1 + hy; vertices[0].color = color; vertices[0].tex_coord = {0,0};
    vertices[1].position.x = x2 + hx; vertices[1].position.y = y2 + hy; vertices[1].color = color; vertices[1].tex_coord = {1,0};
    vertices[2].position.x = x2 - hx; vertices[2].position.y = y2 - hy; vertices[2].color = color; vertices[2].tex_coord = {1,1};
    vertices[3].position.x = x1 - hx; vertices[3].position.y = y1 - hy; vertices[3].color = color; vertices[3].tex_coord = {0,1};

    // Two triangles to form the quad
    int indices[6] = {0, 1, 2, 2, 3, 0};

    SDL_RenderGeometry(renderer, nullptr, vertices, 4, indices, 6);
}

inline bool pointInRect(const SDL_FRect &rect, float x, float y) {
    float localX = x - rect.x;
    float localY = y - rect.y;
    if (localX < 0 || localY < 0) return false;
    if (localX > rect.w || localY > rect.h) return false;
    return true;
}

inline std::string fmt(const char* fmt_str, ...) {
    char buf[256];  // temporary stack buffer

    va_list args;
    va_start(args, fmt_str);
    vsnprintf(buf, sizeof(buf), fmt_str, args);
    va_end(args);

    return {buf};  // copy into std::string
}

inline std::string clean_part(const std::string& part, bool first) {
    size_t start = 0;
    size_t end = part.size();

    // Handle drive specifier (e.g. "C:")
    if (end >= 2 && part[1] == ':') {
        start = 2;
    }

    // Trim leading slashes/backslashes
    while (start < end && (part[start] == '/' || part[start] == '\\')) {
        ++start;
    }

    // Trim trailing slashes/backslashes
    while (end > start && (part[end - 1] == '/' || part[end - 1] == '\\')) {
        --end;
    }

    std::string result = part.substr(start, end - start);

    if (first && part.starts_with(ROOT)) {
        result = ROOT + result;
    }

    return result;
}

inline std::string path(const std::vector<std::string>& parts) {
    std::string result;

    for (size_t i = 0; i < parts.size(); ++i) {
        std::string clean = clean_part(parts[i], i == 0);
        if (clean.empty()) continue;
        if (!result.empty()) result += PATH_SEPARATOR;
        result += clean;
    }

    return result;
}

template<typename... Args>
std::string path(Args&... args) {
    return path(std::vector<std::string>{args...});
}

inline int parseInt(const char* str) {
    int value = 0;
    auto [ptr, ec] = std::from_chars(str, str + std::strlen(str), value);
    if (ec != std::errc()) {
        return -1; // or handle error
    }
    return value;
}

inline const char* getUserHome() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = std::getenv("USERPROFILE"); // Windows fallback
    }
    if (!home) {
        home = ""; // ultimate fallback
    }
    return home;
}

inline int GetTextWidth(TTF_Font* font, const char* str) {
    if (!font || !str || !*str) return 0;

    SDL_Color color = {255, 255, 255, 255}; // opaque white
    SDL_Surface* surface = TTF_RenderText_Blended(font, str, 0, color);
    if (!surface) {
        SDL_Log("TTF_RenderText_Blended failed: %s", SDL_GetError());
        return 0;
    }

    int w = surface->w;
    SDL_DestroySurface(surface); // free memory

    return w;
}

inline double DrawPieSlice(SDL_Renderer* renderer, std::vector<SDL_Vertex>* vb, int cx, int cy, int radius, double percent, SDL_FColor color, double startAngle = -M_PI / 2) {
    if (percent <= 0.0) return startAngle;
    if (percent > 1.0) percent = 1.0;  // clamp

    constexpr int segments = 30;  // increase for smoother slices
    double endAngle = startAngle + 2.0 * M_PI * percent;

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    // Center point
    SDL_Vertex center{};
    center.position.x = static_cast<float>(cx);
    center.position.y = static_cast<float>(cy);
    center.color = color;
    center.tex_coord = {0,0};
    vertices.push_back(center);

    // Arc points
    for (int i = 0; i <= segments; ++i) {
        double theta = startAngle + (endAngle - startAngle) * i / segments;
        SDL_Vertex v{};
        v.position.x = static_cast<float>(cx + radius * cos(theta));
        v.position.y = static_cast<float>(cy + radius * sin(theta));
        v.color = color;
        v.tex_coord = {0,0};
        vertices.push_back(v);
    }

    // Triangle fan indices
    for (int i = 1; i < static_cast<int>(vertices.size()) - 1; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    SDL_RenderGeometry(renderer, nullptr, vertices.data(), static_cast<int>(vertices.size()), indices.data(), static_cast<int>(indices.size()));

    if (vb) {
        *vb = vertices;
    }

    return endAngle;  // return the angle where the next slice should start
}

inline void DrawScrollGradient(SDL_Renderer* renderer, int width, int startX, int startY, int fadeHeight, SDL_Color bg, bool top = false) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Gradient (fade-in/out)
    if (!top) {
        for (int y = 0; y < fadeHeight; y++) {
            const float ratio = static_cast<float>(y) / static_cast<float>(fadeHeight);
            const auto alpha = static_cast<Uint8>(255.0f * ratio);
            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, alpha);
            const auto lineY = static_cast<float>(y + startY);
            SDL_RenderLine(renderer, static_cast<float>(startX), lineY, static_cast<float>(startX + width), lineY);
        }
    } else {
        for (int y = 0; y < fadeHeight; y++) {
            const float ratio = static_cast<float>(y) / static_cast<float>(fadeHeight);
            const auto alpha = static_cast<Uint8>(255.0f - 255.0f * ratio);
            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, alpha);
            const auto lineY = static_cast<float>(y + startY);
            SDL_RenderLine(renderer, static_cast<float>(startX), lineY, static_cast<float>(startX + width), lineY);
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

inline unsigned long CombineIntx2(int a, int b) {
    return (static_cast<unsigned long>(b) << 32) | static_cast<unsigned long>(a);
}

inline int DecombineIntx2(unsigned long combined, int select) {
    return select ? static_cast<int>(combined >> 32)
                  : static_cast<int>(combined & 0xFFFFFFFF);
}

inline unsigned long CalculateThumbHeightAndY(int scrollbarY, int trackHeight, float scrollPosition, float scrollRange) {
    int barMinHeight = 20;

    int barHeight = max(static_cast<int>(1.0f / scrollRange * static_cast<float>(trackHeight)), barMinHeight);
    int barY = scrollbarY + static_cast<int>(scrollPosition * static_cast<float>(trackHeight - barHeight));

    return CombineIntx2(barHeight, barY);
}

inline void DrawScrollbar(SDL_Renderer* renderer, int scrollbarX, int scrollbarY, int height,
                          float scrollPosition, float scrollRange)
{
    constexpr int barMinHeight = 20;
    constexpr int scrollbarThickness = 10;

    int trackHeight = height;

    // Compute thumb height and Y pos
    int barHeight = max(static_cast<int>(1.0f / scrollRange * static_cast<float>(trackHeight)), barMinHeight);
    int barY = scrollbarY + static_cast<int>(scrollPosition * static_cast<float>(trackHeight - barHeight));

    // Draw the track as a thick vertical line
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    DrawThickLineHV(renderer, scrollbarX, scrollbarY, scrollbarX, scrollbarY + trackHeight, scrollbarThickness, true);

    // Draw the thumb as a thick vertical line
    SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);
    DrawThickLineHV(renderer, scrollbarX, barY, scrollbarX, barY + barHeight, scrollbarThickness, true);
}

inline bool PointInTriangle(float px, float py, const SDL_Vertex& v0, const SDL_Vertex& v1, const SDL_Vertex& v2) {
    float x0 = v0.position.x, y0 = v0.position.y;
    float x1 = v1.position.x, y1 = v1.position.y;
    float x2 = v2.position.x, y2 = v2.position.y;

    float dX = px - x2;
    float dY = py - y2;
    float dX21 = x2 - x1;
    float dY12 = y1 - y2;
    float D = dY12*(x0 - x2) + dX21*(y0 - y2);
    float s = dY12*dX + dX21*dY;
    float t = (y2 - y0)*dX + (x0 - x2)*dY;

    if (D < 0) return (s <= 0) && (t <= 0) && (s + t >= D);
    return (s >= 0) && (t >= 0) && (s + t <= D);
}

inline bool PointInVB(float px, float py, const std::vector<SDL_Vertex>& vb) {
    if (vb.size() < 3) return false; // need at least one triangle

    // Triangle fan: first vertex is center
    for (size_t i = 1; i < vb.size() - 1; ++i) {
        if (PointInTriangle(px, py, vb[0], vb[i], vb[i + 1])) {
            return true;
        }
    }
    return false;
}

inline string resourcePath() {
    return {SDL_GetBasePath()};
}

inline string resource(const string &resourceRelPath) {
    return resourcePath() + resourceRelPath;
}

inline TTF_Font* loadFont(float ptsize, SDL_Renderer* renderer) {
    #ifdef __APPLE__
        string resources = resourcePath();
        string fontpath = resources + "JetBrainsMono-Regular.ttf";

        return TTF_OpenFont(fontpath.c_str(), ptsize);
    #endif
}

inline float distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float ilerp(float a, float b, float value) {
    if (a == b) return 0.0f;
    return (value - a) / (b - a);
}

inline float clamp(float value, float minimum, float maximum) {
    return max(minimum, min(value, maximum));
}

#endif // BUDGET_HELPERS_H
