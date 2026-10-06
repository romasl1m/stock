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

    rc = sqlite3_exec(db,
                      "CREATE TABLE IF NOT EXISTS GAME_USERS ("
                      "GAME_ID INTEGER NOT NULL,"
                      "USER_ID INTEGER NOT NULL,"
                      "PRIMARY KEY (GAME_ID, USER_ID),"
                      "FOREIGN KEY (GAME_ID) REFERENCES GAMES(ID),"
                      "FOREIGN KEY (USER_ID) REFERENCES USERS(ID));",
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
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, gm.id),
        sqlite3_bind_int(stmt, 2, gm.start_time),
        sqlite3_bind_int(stmt, 3, gm.end_time),
        sqlite3_bind_text(stmt, 4, gm.password.c_str(), -1, SQLITE_STATIC),
        sqlite3_bind_double(stmt, 5, gm.starting_balance);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "DB insert error\n";
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    sqlite3_finalize(stmt);

    for (const auto &usr : gm.users) {
        string usr_sql = "INSERT INTO GAME_USERS (GAME_ID, USER_ID) VALUES (?, ?);";
        sqlite3_stmt *usr_stmt;
        if (sqlite3_prepare_v2(db, usr_sql.c_str(), -1, &usr_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(usr_stmt, 1, gm.id),
                sqlite3_bind_int(usr_stmt, 2, usr.id),
                sqlite3_step(usr_stmt),
                sqlite3_finalize(usr_stmt);
        }
    }

    sqlite3_close(db);
}

void end_game(const game &gm) {
    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }
    ensure_game_tables(db);

    string del_users_sql = "DELETE FROM GAME_USERS WHERE GAME_ID = ?;";
    sqlite3_stmt *usr_stmt;
    if (sqlite3_prepare_v2(db, del_users_sql.c_str(), -1, &usr_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(usr_stmt, 1, gm.id),
            sqlite3_step(usr_stmt),
            sqlite3_finalize(usr_stmt);
    }

    string sql = "DELETE FROM GAMES WHERE ID = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, gm.id),
        rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        cerr << "DB delete error\n";
    sqlite3_finalize(stmt),
        sqlite3_close(db);
}

void join_game(game &gm, const user &usr) {
    sqlite3 *db;
    int rc = sqlite3_open("trading.db", &db);
    if (rc) {
        cerr << "DB error\n";
        return;
    }
    ensure_game_tables(db);

    string sql = "INSERT INTO GAME_USERS (GAME_ID, USER_ID) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "DB error while preparing the request\n";
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, gm.id),
        sqlite3_bind_int(stmt, 2, usr.id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        cerr << "DB insert error\n";
    else
        gm.users.push_back(usr);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
