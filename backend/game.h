#ifndef GAME_H
#define GAME_H

#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <unordered_map>
using namespace std;

struct game {
    int id, start_time, end_time;
    bool status;
    string password;
    double starting_balance;
};

#endif
