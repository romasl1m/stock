#ifndef USER_H
#define USER_H

#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <unordered_map>
using namespace std;

struct user {
    int id;
    double balance;
    string name, description, password;
    unordered_map<int, int> usr_stocks;
};

void check_error(int rc, char *errmsg, sqlite3 *db);
void ensure_user_tables(sqlite3 *db);
void add_user_to_db(const user &usr);
vector<user> get_users();
user get_user_by_id(const int &id);
double get_portfolio_value(const int &id);

#endif
