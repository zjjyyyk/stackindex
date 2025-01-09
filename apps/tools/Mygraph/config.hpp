// #pragma once

#include <iostream>
#include <unordered_map>

struct DatasetSetting {
    bool directed;
    int skipstart;
    int beginnode;
    bool appear_twice;
};

std::unordered_map<std::string,DatasetSetting> DATASET_SETTING = {
    {"road-CA1.txt", {false, 0, 1, true}},
    {"road-PA1.txt", {false, 0, 1, true}},
    {"com-hep-th1.txt", {false, 0, 1, true}},
    {"orkut.txt", {false, 1, 0, false}},
    {"pokec.txt", {false, 1, 0, false}},
    {"youtube.txt", {false, 1, 0, false}},
    {"astro-ph.txt", {false, 0, 0, true}},
    {"email-enron.txt", {false, 0, 0, true}},
    {"road-powergrid.txt", {false, 0, 1, false}},
};

