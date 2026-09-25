//
// Created by Ayaan Banday on 2025-10-13.
//

#ifndef BUDGET_SCHEDULE_H
#define BUDGET_SCHEDULE_H

#include <filesystem>
using namespace std;
namespace fs = filesystem;

struct BDBYear {
    int year;
    bool months[12];
};

struct BDBTimes {
    BDBYear years[32]; // every 32 years
};

inline BDBTimes getAllTimes(const std::string& folder) {
    BDBTimes times{};
    int i = 0;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().filename().string() == ".DS_Store") continue;

        int year, month;
        std::string filename = entry.path().filename().string();

        if (sscanf(filename.c_str(), "%d-%d.bin", &year, &month) != 2) { // NOLINT(*-err34-c)
            printf("Uh oh, Seems like sscanf didn't work. sscanf(%s, \"%%d-%%d.bin\", &year, &month)\n", filename.c_str());
            abort();
        }
        month--; // month starts at 1, we need 0

        // check if this year already exists
        bool found = false;
        for (int j = 0; j < i; j++) {
            if (times.years[j].year == year) {
                times.years[j].months[month] = true;
                found = true;
                break;
            }
        }

        if (!found) {
            times.years[i].year = year;
            times.years[i].months[month] = true;
            i++;
        }
    }

    sort(times.years, times.years + i, [](const BDBYear &a, const BDBYear &b) {
        return a.year < b.year;
    });

    return times;
}

#endif //BUDGET_SCHEDULE_H