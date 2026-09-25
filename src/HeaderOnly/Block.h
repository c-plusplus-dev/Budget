//
// Created by Ayaan Banday on 2025-09-21.
//

#ifndef BUDGET_ITEM_H
#define BUDGET_ITEM_H

#include <fstream>

#include "Helpers.h"

struct Item {
    char name[64];
    int cost;
    bool blank = true;
};

struct Block {
    char name[64];
    Item items[128];
    bool blank = true;
    bool expanded = false;
};

// bookmark: start of block header
inline Block CreateBlock(const char* name) {
    Block block{};
    std::strncpy(block.name, name, sizeof(block.name) - 1);
    block.name[sizeof(block.name) - 1] = '\0'; // ensure null-termination
    block.blank = false;
    return block;
}

inline void DeleteBlock(Block &block) {
    block.blank = true;
    for (Item &item : block.items) {
        item.blank = true;
        item.cost = 0;

        strncpy(item.name, "", sizeof(item.name) - 1);
        item.name[sizeof(item.name) - 1] = '\0'; // ensure null-termination
    }
    strncpy(block.name, "", sizeof(block.name) - 1);
    block.name[sizeof(block.name) - 1] = '\0'; // ensure null-termination

    block.expanded = false;
}

inline int CalculateTotalCostOfBlock(Block &block) {
    if (block.blank) return 0;
    int cost = 0;
    for (Item &item : block.items) {
        if (item.blank) continue;
        cost += item.cost;
    }
    return cost;
}

inline int CalculateTotalCostOfBlocks(Block blocks[], int length) {
    int total_cost = 0;
    for (int i = 0; i < length; i++) {
        Block block = blocks[i];
        int cost = 0;
        for (Item &item : block.items) {
            if (item.blank) continue;
            cost += item.cost;
        }
        total_cost += cost;
    }
    return total_cost;
}

inline int FindFirstEmptyBlock(Block blocks[], int length) {
    for (int i = 0; i < length; i++) {
        if (blocks[i].blank) return i;
    }
    return -1;
}

inline bool WriteBlocksToFile(Block blocks[], int length, const char* filename) {
    // Make sure all blocks are unexpanded
    for (int i = 0; i < length; i++) {
        blocks[i].expanded = false;
    }

    // Open file in binary mode
    std::ofstream out(filename, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    // Write the raw block data
    out.write(reinterpret_cast<const char*>(blocks), static_cast<streamsize>(sizeof(Block) * length));
    if (!out.good()) {
        return false;
    }

    out.close();
    return true;
}

inline Block* ReadBlocksFromFile(Block* blocks, int length, const char* filename) {
    // Open the file in binary mode
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return nullptr;

    // Allocate memory if blocks pointer is null
    if (!blocks) {
        blocks = static_cast<Block*>(malloc(sizeof(Block) * length));
        if (!blocks) return nullptr; // allocation failed
    }

    // Read the raw block data
    in.read(reinterpret_cast<char*>(blocks), static_cast<streamsize>(sizeof(Block) * length));
    if (!in) { // check if read failed or didn't read enough
        free(blocks); // avoid memory leak if we allocated
        return nullptr;
    }

    in.close();
    return blocks;
}

// Hash a string into a 32-bit integer
static uint32_t HashString(const char* str) {
    uint32_t hash = 2166136261u; // FNV-1a
    while (*str) {
        hash ^= static_cast<unsigned char>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

// Convert a Block into a unique color
inline SDL_Color BlockToColor(const Block& block) {
    // Use the block name as the seed
    uint32_t hash = HashString(block.name);

    // Extract RGB channels from the hash
    Uint8 r = hash & 0xFF;
    Uint8 g = hash >> 8 & 0xFF;
    Uint8 b = hash >> 16 & 0xFF;

    // Optionally brighten the color (avoid very dark colors)
    constexpr int minBrightness = 64;
    r = static_cast<Uint8>(minBrightness + r / 2);
    g = static_cast<Uint8>(minBrightness + g / 2);
    b = static_cast<Uint8>(minBrightness + b / 2);

    return SDL_Color{r, g, b, 255};
}

inline int GetBlockIndexWithMostTotalCost(Block blocks[], int length) {
    int maxCost = 0;
    int index = 0;
    for (int i = 0; i < length; i++) {
        if (blocks[i].blank) continue;
        if (CalculateTotalCostOfBlock(blocks[i]) > maxCost) {
            maxCost = CalculateTotalCostOfBlock(blocks[i]);
            index = i;
        }
    }
    return index;
}

inline Block* GetBlockByName(Block blocks[], int length, const char* name) {
    for (int i = 0; i < length; i++) {
        if (blocks[i].blank) continue;
        if (strcmp(blocks[i].name, name) == 0) {
            return &blocks[i];
        }
    }
    return nullptr;
}

// bookmark: start of item header
inline Item CreateItem(const char* name, int cost) {
    Item item{};
    std::strncpy(item.name, name, sizeof(item.name) - 1);
    item.name[sizeof(item.name) - 1] = '\0'; // ensure null-termination
    item.cost = cost;
    item.blank = false;
    return item;
}

inline void DeleteItem(Item &item) {
    item.blank = true;
    item.cost = 0;

    strncpy(item.name, "", sizeof(item.name) - 1);
    item.name[sizeof(item.name) - 1] = '\0'; // ensure null-termination
}

inline int FindFirstEmptyItem(Block block) {
    for (int i = 0; i < 128; i++) {
        Item &item = block.items[i];
        if (item.blank) return i;
    }
    return -1;
}

#endif // BUDGET_ITEM_H
