#include "game.h"
#include "user.h"
#include "stock.h"

void ensure_game_tables(sqlite3 *db) {
    char *errmsg = nullptr;

    int rc = sqlite3_exec(db,
                          "CREATE TABLE IF NOT EXISTS GAMES ("
                          "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "START_TIME DATETIME DEFAULT CURRENT_TIMESTAMP,"
                          "END_TIME DATETIME DEFAULT CURRENT_TIMESTAMP,"
                          "PASSWORD TEXT NOT NULL,"
                          "BALANCE REAL);",
                          nullptr, nullptr, &errmsg);
    check_error(rc, errmsg, db);
}

void create_game(const game &gm) {
    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }
    ensure_game_tables(db);

    string sql = "INSERT INTO GAMES (ID, START_TIME, END_TIME, PASSWORD, BALANCE) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request";
        sqlite3_close(db);
        return;
    }

    // sqlite3_bind_int(stmt, 1, gm.id),
    //     sqlite3_bind_int(stmt, 2, gm, starting_time),
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        cerr << "DB insert error\n";
    sqlite3_finalize(stmt), sqlite3_close(db);
}

void end_game() {
}

void join_game() {
}
