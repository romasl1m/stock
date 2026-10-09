#ifndef GAME_H
#define GAME_H

#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <unordered_map>
#include "user.h"
using namespace std;

struct game {
    int id, start_time, end_time;
    int status; // 0 not started, 1 started, 2 ended.
    string password;
    double starting_balance;
    vector<user> users;
};

void ensure_game_tables(sqlite3 *db);
void create_game(const game &gm);
void end_game(const game &gm);
void join_game(game &gm, const user &usr);
void start_game(const game &gm);

#endif
