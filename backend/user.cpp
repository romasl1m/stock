#include "user.h"
#include "stock.h"

void check_error(int rc, char *errmsg, sqlite3 *db) {
    if (rc != SQLITE_OK)
        cerr << "DB error " << errmsg << "\n", sqlite3_free(errmsg), sqlite3_close(db), exit(1);
}

static void load_usr_stocks(sqlite3 *db, user &usr) {
    string sql = "SELECT STOCK_ID, QUANTITY FROM USER_STOCKS WHERE USER_ID = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_int(stmt, 1, usr.id);

    while (sqlite3_step(stmt) == SQLITE_ROW)
        usr.usr_stocks[sqlite3_column_int(stmt, 0)] = sqlite3_column_int(stmt, 1);
    sqlite3_finalize(stmt);
}

void ensure_user_tables(sqlite3 *db) {
    char *errmsg = nullptr;

    int rc = sqlite3_exec(db,
                          "CREATE TABLE IF NOT EXISTS USERS ("
                          "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "BALANCE REAL,"
                          "NAME TEXT NOT NULL,"
                          "DESCRIPTION TEXT NOT NULL,"
                          "PASSWORD TEXT NOT NULL);",
                          nullptr, nullptr, &errmsg);
    check_error(rc, errmsg, db);

    rc = sqlite3_exec(db,
                      "CREATE TABLE IF NOT EXISTS USER_STOCKS ("
                      "USER_ID INTEGER NOT NULL,"
                      "STOCK_ID INTEGER NOT NULL,"
                      "QUANTITY INTEGER NOT NULL,"
                      "PRIMARY KEY (USER_ID, STOCK_ID));",
                      nullptr, nullptr, &errmsg);
    check_error(rc, errmsg, db);

    rc = sqlite3_exec(db,
                      "CREATE TABLE IF NOT EXISTS TRANSACTIONS ("
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "USER_ID INTEGER NOT NULL,"
                      "STOCK_ID INTEGER NOT NULL,"
                      "TYPE TEXT NOT NULL,"
                      "QUANTITY INTEGER NOT NULL,"
                      "PRICE REAL NOT NULL,"
                      "TIMESTAMP DATETIME DEFAULT CURRENT_TIMESTAMP,"
                      "FOREIGN KEY (USER_ID) REFERENCES USERS(ID),"
                      "FOREIGN KEY (STOCK_ID) REFERENCES STOCKS(ID));",
                      nullptr, nullptr, &errmsg);
    check_error(rc, errmsg, db);
}

void add_user_to_db(const user &usr) {
    sqlite3 *db;

    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }

    ensure_user_tables(db);

    string sql = "INSERT INTO USERS (ID, BALANCE, NAME, DESCRIPTION, PASSWORD) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, usr.id),
        sqlite3_bind_double(stmt, 2, usr.balance),
        sqlite3_bind_text(stmt, 3, usr.name.c_str(), -1, SQLITE_STATIC),
        sqlite3_bind_text(stmt, 4, usr.description.c_str(), -1, SQLITE_STATIC),
        sqlite3_bind_text(stmt, 5, usr.password.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        cerr << "DB insert error\n";
    sqlite3_finalize(stmt);

    for (const auto &[stock_id, quantity] : usr.usr_stocks) {
        string stk_sql = "INSERT OR REPLACE INTO USER_STOCKS (USER_ID, STOCK_ID, QUANTITY) VALUES (?, ?, ?);";
        sqlite3_stmt *stk_stmt;
        if (sqlite3_prepare_v2(db, stk_sql.c_str(), -1, &stk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stk_stmt, 1, usr.id),
                sqlite3_bind_int(stk_stmt, 2, stock_id),
                sqlite3_bind_int(stk_stmt, 3, quantity),
                sqlite3_step(stk_stmt),
                sqlite3_finalize(stk_stmt);
        }
    }

    sqlite3_close(db);
}

vector<user> get_users() {
    sqlite3 *db;

    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return {};
    }

    ensure_user_tables(db);

    sqlite3_stmt *stmt;
    string sql = "SELECT id, balance, name, description, password FROM USERS";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return {};
    }

    vector<user> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        user usr;

        usr.id = sqlite3_column_int(stmt, 0);
        usr.balance = sqlite3_column_double(stmt, 1);

        const char *name = (const char *)sqlite3_column_text(stmt, 2),
                   *desc = (const char *)sqlite3_column_text(stmt, 3),
                   *pass = (const char *)sqlite3_column_text(stmt, 4);
        usr.name = name ? name : "",
        usr.description = desc ? desc : "",
        usr.password = pass ? pass : "";

        load_usr_stocks(db, usr);
        users.push_back(usr);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return users;
}

user get_user_by_id(const int &id) {
    sqlite3 *db;

    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return {};
    }

    ensure_user_tables(db);

    string sql = "SELECT id, balance, name, description, password FROM users WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return {};
    }
    sqlite3_bind_int(stmt, 1, id);

    user usr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        usr.id = sqlite3_column_int(stmt, 0);
        usr.balance = sqlite3_column_double(stmt, 1);

        const char *name = (const char *)sqlite3_column_text(stmt, 2);
        const char *desc = (const char *)sqlite3_column_text(stmt, 3);
        const char *pass = (const char *)sqlite3_column_text(stmt, 4);
        usr.name = name ? name : "";
        usr.description = desc ? desc : "";
        usr.password = pass ? pass : "";

        load_usr_stocks(db, usr);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return usr;
}

double get_portfolio_value(const int &id) {
    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return 0.0;
    }

    ensure_user_tables(db),
        ensure_stock_tables(db);

    string sql = "SELECT SUM(us.QUANTITY * s.COST) FROM USER_STOCKS us "
                 "JOIN STOCKS s ON us.STOCK_ID = s.ID "
                 "WHERE us.USER_ID = ?";
    sqlite3_stmt *stmt;
    double value = 0.0;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW and sqlite3_column_type(stmt, 0) != SQLITE_NULL)
            value = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return value;
}
