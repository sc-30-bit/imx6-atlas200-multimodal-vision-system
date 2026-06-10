#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

#define DB_PATH "/mnt/boa/database/analysis.db"

struct AnalysisRecord {
    int id;
    std::string ip;
    std::string prompt;
    std::string result;
    double cost_time;
    std::string created_at;
    std::string image;
};

class Database {
public:
    Database() : m_db(nullptr) {}

    ~Database() {
        if (m_db) sqlite3_close(m_db);
    }

    bool open(const std::string& path = DB_PATH)
    {
        mkdir("/mnt/boa/database", 0755);
        int rc = sqlite3_open(path.c_str(), &m_db);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "DB open error: %s\n", sqlite3_errmsg(m_db));
            return false;
        }
        return createTable();
    }

    bool insertRecord(const std::string& ip, const std::string& prompt,
                      const std::string& result, double costTime,
                      const std::string& image = "")
    {
        if (!m_db) return false;
        const char* sql =
            "INSERT INTO analysis_records (ip, prompt, result, cost_time, image) "
            "VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "DB prepare error: %s\n", sqlite3_errmsg(m_db));
            return false;
        }
        sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, prompt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, result.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, costTime);
        sqlite3_bind_text(stmt, 5, image.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }

    std::vector<AnalysisRecord> getAllRecords(int limit = 50)
    {
        std::vector<AnalysisRecord> records;
        if (!m_db) return records;

        const char* sql =
            "SELECT id, ip, prompt, result, cost_time, created_at, image "
            "FROM analysis_records ORDER BY id DESC LIMIT ?;";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) return records;

        sqlite3_bind_int(stmt, 1, limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AnalysisRecord r;
            r.id = sqlite3_column_int(stmt, 0);
            const char* ip = (const char*)sqlite3_column_text(stmt, 1);
            const char* prompt = (const char*)sqlite3_column_text(stmt, 2);
            const char* result = (const char*)sqlite3_column_text(stmt, 3);
            r.cost_time = sqlite3_column_double(stmt, 4);
            const char* created = (const char*)sqlite3_column_text(stmt, 5);
            const char* image = (const char*)sqlite3_column_text(stmt, 6);
            r.ip = ip ? ip : "";
            r.prompt = prompt ? prompt : "";
            r.result = result ? result : "";
            r.created_at = created ? created : "";
            r.image = image ? image : "";
            records.push_back(r);
        }
        sqlite3_finalize(stmt);
        return records;
    }

private:
    bool createTable()
    {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS analysis_records ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "ip TEXT NOT NULL,"
            "prompt TEXT,"
            "result TEXT,"
            "cost_time REAL,"
            "created_at TIMESTAMP DEFAULT (datetime('now','localtime')),"
            "image TEXT"
            ");";
        char* errMsg = nullptr;
        int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "DB create table error: %s\n", errMsg);
            sqlite3_free(errMsg);
            return false;
        }

        sqlite3_exec(m_db,
            "ALTER TABLE analysis_records ADD COLUMN image TEXT",
            nullptr, nullptr, nullptr);

        return true;
    }

    sqlite3* m_db;
};

#endif
