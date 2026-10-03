#include "user.h"
#include "stock.h"
using namespace std;

void ensure_stock_tables(sqlite3 *db) {
    char *errmsg = nullptr;
    int rc = sqlite3_exec(db,
                          "CREATE TABLE IF NOT EXISTS STOCKS ("
                          "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "COST REAL,"
                          "NAME TEXT NOT NULL,"
                          "DESCRIPTION TEXT NOT NULL);",
                          nullptr, nullptr, &errmsg);
    check_error(rc, errmsg, db);
}

void add_stock_to_db(const stock &stk) {
    sqlite3 *db;

    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }

    ensure_stock_tables(db);

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

    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return {};
    }

    ensure_stock_tables(db);

    sqlite3_stmt *stmt;
    string sql = "SELECT id, cost, name, description FROM STOCKS";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return {};
    }

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

stock get_stock_by_id(const int &id) {
    sqlite3 *db;

    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return {};
    }

    ensure_stock_tables(db);

    string sql = "SELECT id, cost, name, description FROM stocks WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return {};
    }
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

void buy_stock(const int &usr_id, const int &stk_id, const int &quantity) {
    if (quantity <= 0)
        return;

    user usr = get_user_by_id(usr_id);
    stock stk = get_stock_by_id(stk_id);

    if (usr.id == 0 or stk.id == 0)
        return;

    double price = stk.cost * quantity;
    if (price > usr.balance)
        return;

    usr.balance -= price;
    usr.usr_stocks[stk_id] += quantity;

    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }

    ensure_user_tables(db);

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

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

    string tx_sql = "INSERT INTO TRANSACTIONS (USER_ID, STOCK_ID, TYPE, QUANTITY, PRICE) VALUES (?, ?, 'BUY', ?, ?);";
    sqlite3_stmt *tx_stmt;
    if (sqlite3_prepare_v2(db, tx_sql.c_str(), -1, &tx_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(tx_stmt, 1, usr_id);
        sqlite3_bind_int(tx_stmt, 2, stk_id);
        sqlite3_bind_int(tx_stmt, 3, quantity);
        sqlite3_bind_double(tx_stmt, 4, price);
        sqlite3_step(tx_stmt);
        sqlite3_finalize(tx_stmt);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_close(db);
}

void sell_stock(const int &usr_id, const int &stk_id, const int &quantity) {
    if (quantity <= 0)
        return;

    user usr = get_user_by_id(usr_id);
    stock stk = get_stock_by_id(stk_id);

    if (usr.id == 0 or stk.id == 0)
        return;

    auto it = usr.usr_stocks.find(stk_id);
    if (it == usr.usr_stocks.end() or it->second < quantity)
        return;

    double revenue = 0.81 * stk.cost * (double)quantity;

    usr.balance += revenue,
        it->second -= quantity;

    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }

    ensure_user_tables(db);

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    string bal_sql = "UPDATE USERS SET BALANCE = ? WHERE ID = ?";
    sqlite3_stmt *bal_stmt;
    if (sqlite3_prepare_v2(db, bal_sql.c_str(), -1, &bal_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(bal_stmt, 1, usr.balance);
        sqlite3_bind_int(bal_stmt, 2, usr_id);
        sqlite3_step(bal_stmt);
        sqlite3_finalize(bal_stmt);
    }

    if (it->second == 0) {
        string del_sql = "DELETE FROM USER_STOCKS WHERE USER_ID = ? AND STOCK_ID = ?;";
        sqlite3_stmt *del_stmt;
        if (sqlite3_prepare_v2(db, del_sql.c_str(), -1, &del_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(del_stmt, 1, usr_id);
            sqlite3_bind_int(del_stmt, 2, stk_id);
            sqlite3_step(del_stmt);
            sqlite3_finalize(del_stmt);
        }
    } else {
        string stk_sql = "UPDATE USER_STOCKS SET QUANTITY = ? WHERE USER_ID = ? AND STOCK_ID = ?;";
        sqlite3_stmt *stk_stmt;
        if (sqlite3_prepare_v2(db, stk_sql.c_str(), -1, &stk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stk_stmt, 1, it->second);
            sqlite3_bind_int(stk_stmt, 2, usr_id);
            sqlite3_bind_int(stk_stmt, 3, stk_id);
            sqlite3_step(stk_stmt);
            sqlite3_finalize(stk_stmt);
        }
    }

    string tx_sql = "INSERT INTO TRANSACTIONS (USER_ID, STOCK_ID, TYPE, QUANTITY, PRICE) VALUES (?, ?, 'SELL', ?, ?);";
    sqlite3_stmt *tx_stmt;
    if (sqlite3_prepare_v2(db, tx_sql.c_str(), -1, &tx_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(tx_stmt, 1, usr_id);
        sqlite3_bind_int(tx_stmt, 2, stk_id);
        sqlite3_bind_int(tx_stmt, 3, quantity);
        sqlite3_bind_double(tx_stmt, 4, revenue);
        sqlite3_step(tx_stmt);
        sqlite3_finalize(tx_stmt);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_close(db);
}

void update_stocks(const int &growth) {
    srand(time(NULL));
    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }

    ensure_stock_tables(db);

    vector<stock> stocks = get_stocks();

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (auto &stk : stocks) {
        double pct = (rand() % ((growth << 1) | 1) - growth) / 100.0;
        stk.cost *= (1.0 + pct);
        stk.cost = max(stk.cost, 0.01);

        string sql = "UPDATE STOCKS SET COST = ? WHERE ID = ?;";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_double(stmt, 1, stk.cost);
            sqlite3_bind_int(stmt, 2, stk.id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_close(db);
}
