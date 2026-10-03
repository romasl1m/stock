#ifndef STOCK_H
#define STOCK_H
#include <iostream>
#include <sqlite3.h>
#include "user.h"
using namespace std;

struct stock {
    int id;
    double cost;
    string name, description;
};

void ensure_stock_tables(sqlite3 *db);
void add_stock_to_db(const stock &stk);
vector<stock> get_stocks();
stock get_stock_by_id(const int &id);
void buy_stock(const int &usr_id, const int &stk_id, const int &quantity);
void sell_stock(const int &usr_id, const int &stk_id, const int &quantity);
void update_stocks(const int &growth);

#endif
