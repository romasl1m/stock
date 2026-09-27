#include <iostream>
#include <sqlite3.h>
#include "user.cpp"
using namespace std;

struct stock {
    int id;
    double cost;
    string name, description;
};

void add_stock_to_db(const stock &stk) {
    sqlite3 *db;
    char *errmsg = nullptr;

    int rc = sqlite3_open("stocks.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }
    string sql_stocks =
        "CREATE TABLE IF NOT EXISTS STOCKS ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "COST REAL,"
        "NAME TEXT NOT NULL,"
        "DESCRIPTION TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql_stocks.c_str(), nullptr, nullptr, &errmsg);
    check_error(rc, errmsg, db);

    string sql = "INSERT INTO STOCKS (ID, COST, NAME, DESCRIPTION) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, stk.id);
    sqlite3_bind_double(stmt, 2, stk.cost);
    sqlite3_bind_text(stmt, 3, stk.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, stk.description.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        cerr << "DB insert error\n";

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

vector<stock> get_stocks() {
    sqlite3 *db;

    int rc = sqlite3_open("stocks.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return {};
    }
    sqlite3_stmt *stmt;
    string sql = "SELECT id, cost, name, description FROM STOCKS";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    vector<stock> stocks;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        stock stk;

        stk.id = sqlite3_column_int(stmt, 0);
        stk.cost = sqlite3_column_double(stmt, 1);

        const char *name = (const char *)sqlite3_column_text(stmt, 2);
        const char *desc = (const char *)sqlite3_column_text(stmt, 3);
        stk.name = name ? name : "";
        stk.description = desc ? desc : "";

        stocks.push_back(stk);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return stocks;
}

stock get_stock_by_id(int id) {
    sqlite3 *db;

    int rc = sqlite3_open("stocks.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return {};
    }
    string sql = "SELECT id, cost, name, description FROM stocks WHERE id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    stock stk;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stk.id = sqlite3_column_int(stmt, 0);
        stk.cost = sqlite3_column_double(stmt, 1);

        const char *name = (const char *)sqlite3_column_text(stmt, 2);
        const char *desc = (const char *)sqlite3_column_text(stmt, 3);
        stk.name = name ? name : "";
        stk.description = desc ? desc : "";
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return stk;
}

void buy_stock(int usr_id, int stk_id, int quantity) {
    user usr = get_user_by_id(usr_id);
    stock stk = get_stock_by_id(stk_id);

    double price = stk.cost * quantity;
    if (price > usr.balance)
        return;

    usr.balance -= price;
    usr.usr_stocks[stk_id] += quantity;

    sqlite3 *db;
    int rc = sqlite3_open("users.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }

    ensure_user_tables(db);

    string bal_sql = "UPDATE USERS SET BALANCE = ? WHERE ID = ?";
    sqlite3_stmt *bal_stmt;
    if (sqlite3_prepare_v2(db, bal_sql.c_str(), -1, &bal_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(bal_stmt, 1, usr.balance);
        sqlite3_bind_int(bal_stmt, 2, usr_id);
        sqlite3_step(bal_stmt);
        sqlite3_finalize(bal_stmt);
    }

    string stk_sql =
        "INSERT INTO USER_STOCKS (USER_ID, STOCK_ID, QUANTITY) VALUES (?, ?, ?)"
        " ON CONFLICT(USER_ID, STOCK_ID) DO UPDATE SET QUANTITY = ?;";
    sqlite3_stmt *stk_stmt;
    if (sqlite3_prepare_v2(db, stk_sql.c_str(), -1, &stk_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stk_stmt, 1, usr_id);
        sqlite3_bind_int(stk_stmt, 2, stk_id);
        sqlite3_bind_int(stk_stmt, 3, usr.usr_stocks[stk_id]);
        sqlite3_bind_int(stk_stmt, 4, usr.usr_stocks[stk_id]);
        sqlite3_step(stk_stmt);
        sqlite3_finalize(stk_stmt);
    }

    sqlite3_close(db);
}
