// settings_server.cpp
//
// Simple Settings Backend in C++
// - HTTP REST server using cpp-httplib (single-header)
// - JSON handling via nlohmann::json (single-header)
// - Persistence in SQLite3 (system library)
// - Endpoints:
//   GET  /settings?user_id=...           -> returns all settings for a user
//   POST /settings                      -> create/update settings (body JSON)
//   POST /profile                       -> update profile (name, email, avatar_url)
//   POST /notifications                 -> update notification granular toggles
//   POST /theme                         -> set theme mode (System/Light/Dark)
//   POST /security/biometric            -> enable/disable biometric lock
//   POST /history/clear                 -> clear chat history
//   GET  /history/export?user_id=...    -> get JSON export of chat history & settings
//   POST /history/import                -> import JSON payload (merge/replace)
//   GET  /health                        -> simple health check
//
// Build (example):
// g++ settings_server.cpp -std=c++17 -O2 -lsqlite3 -pthread -o settings_server
//
// Requirements:
// - httplib.h (cpp-httplib single header) in include path
// - json.hpp (nlohmann/json single header) in include path
// - sqlite3 development library
//
// Notes:
// - This is a minimal but robust starting point. Add TLS, auth tokens, rate limits,
//   validation, and further security for production usage.

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <mutex>

#include "httplib.h"     // https://github.com/yhirose/cpp-httplib (single header)
#include "json.hpp"      // nlohmann::json (single header)
#include <sqlite3.h>

using json = nlohmann::json;
using namespace httplib;

static const std::string API_KEY_HEADER = "X-API-KEY";
static const std::string VALID_API_KEY  = "secret-api-key"; // replace in prod

// Database filename
static const char* DB_FILE = "luma_settings.db";

// Thread-safety for sqlite usage in this example
std::mutex db_mutex;

// Helper: get current ISO timestamp
std::string iso_now() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::string(buf);
}

// --- SQLite helper functions --- //

static int exec_sql(sqlite3* db, const std::string& sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[sqlite] exec error: " << (errmsg ? errmsg : "<null>") << "\n";
        sqlite3_free(errmsg);
    }
    return rc;
}

// Initialize DB: create tables users, settings, chat_history
void init_db() {
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) {
        std::cerr << "Failed to open DB\n";
        return;
    }

    // Users table (basic profile)
    std::string users_sql = R"sql(
    CREATE TABLE IF NOT EXISTS users (
      user_id TEXT PRIMARY KEY,
      name TEXT,
      email TEXT,
      avatar_url TEXT,
      created_at TEXT
    );
    )sql";

    // Settings table (one row per user)
    std::string settings_sql = R"sql(
    CREATE TABLE IF NOT EXISTS settings (
      user_id TEXT PRIMARY KEY,
      theme_mode TEXT DEFAULT 'System', -- System|Light|Dark
      dark_mode INTEGER DEFAULT 0,
      notifications_enabled INTEGER DEFAULT 1,
      chat_notifications INTEGER DEFAULT 1,
      update_notifications INTEGER DEFAULT 1,
      reminder_notifications INTEGER DEFAULT 0,
      language TEXT DEFAULT 'English',
      biometric_lock INTEGER DEFAULT 0,
      app_version TEXT DEFAULT '1.0.0',
      updated_at TEXT,
      FOREIGN KEY(user_id) REFERENCES users(user_id)
    );
    )sql";

    // Chat history
    std::string history_sql = R"sql(
    CREATE TABLE IF NOT EXISTS chat_history (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      user_id TEXT,
      role TEXT, -- user | bot
      message TEXT,
      created_at TEXT,
      FOREIGN KEY(user_id) REFERENCES users(user_id)
    );
    )sql";

    exec_sql(db, users_sql);
    exec_sql(db, settings_sql);
    exec_sql(db, history_sql);

    sqlite3_close(db);
}

// Ensure user exists in users/settings (create default rows)
void ensure_user_exists(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) return;

    // begin transaction
    exec_sql(db, "BEGIN TRANSACTION;");

    // Insert into users if not exists
    {
        std::string sql = "INSERT OR IGNORE INTO users(user_id, name, email, avatar_url, created_at) VALUES(?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, "User Name", -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, "user@example.com", -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, "", -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, iso_now().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    // Insert default settings if not exists
    {
        std::string sql = R"sql(
        INSERT OR IGNORE INTO settings(
          user_id, theme_mode, dark_mode, notifications_enabled,
          chat_notifications, update_notifications, reminder_notifications,
          language, biometric_lock, app_version, updated_at
        ) VALUES(?, 'System', 0, 1, 1, 1, 0, 'English', 0, '1.0.0', ?);
        )sql";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, iso_now().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    exec_sql(db, "COMMIT;");
    sqlite3_close(db);
}

// Fetch settings as JSON
json get_user_settings(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    ensure_user_exists(user_id);

    sqlite3* db = nullptr;
    json out;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) return out;

    std::string sql = R"sql(
      SELECT u.user_id, u.name, u.email, u.avatar_url,
             s.theme_mode, s.dark_mode, s.notifications_enabled,
             s.chat_notifications, s.update_notifications, s.reminder_notifications,
             s.language, s.biometric_lock, s.app_version, s.updated_at
      FROM users u
      JOIN settings s ON u.user_id = s.user_id
      WHERE u.user_id = ?;
    )sql";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out["user_id"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            out["name"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            out["email"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            out["avatar_url"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            out["theme_mode"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            out["dark_mode"] = sqlite3_column_int(stmt, 5) != 0;
            out["notifications_enabled"] = sqlite3_column_int(stmt, 6) != 0;
            out["chat_notifications"] = sqlite3_column_int(stmt, 7) != 0;
            out["update_notifications"] = sqlite3_column_int(stmt, 8) != 0;
            out["reminder_notifications"] = sqlite3_column_int(stmt, 9) != 0;
            out["language"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
            out["biometric_lock"] = sqlite3_column_int(stmt, 11) != 0;
            out["app_version"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            out["updated_at"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return out;
}

// Update settings given JSON (partial allowed)
bool upsert_settings(const std::string& user_id, const json& j) {
    std::lock_guard<std::mutex> lock(db_mutex);
    ensure_user_exists(user_id);

    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) return false;
    exec_sql(db, "BEGIN TRANSACTION;");

    // Update users if profile keys present
    if (j.contains("name") || j.contains("email") || j.contains("avatar_url")) {
        std::string sql = "UPDATE users SET name = COALESCE(?, name), email = COALESCE(?, email), avatar_url = COALESCE(?, avatar_url) WHERE user_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            // bind in order: name, email, avatar, user_id
            if (j.contains("name")) sqlite3_bind_text(stmt, 1, j["name"].get<std::string>().c_str(), -1, SQLITE_TRANSIENT);
            else sqlite3_bind_null(stmt, 1);
            if (j.contains("email")) sqlite3_bind_text(stmt, 2, j["email"].get<std::string>().c_str(), -1, SQLITE_TRANSIENT);
            else sqlite3_bind_null(stmt, 2);
            if (j.contains("avatar_url")) sqlite3_bind_text(stmt, 3, j["avatar_url"].get<std::string>().c_str(), -1, SQLITE_TRANSIENT);
            else sqlite3_bind_null(stmt, 3);
            sqlite3_bind_text(stmt, 4, user_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    // Build settings update
    std::string sql = R"sql(
      INSERT INTO settings(user_id, theme_mode, dark_mode, notifications_enabled,
        chat_notifications, update_notifications, reminder_notifications,
        language, biometric_lock, app_version, updated_at)
      VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
      ON CONFLICT(user_id) DO UPDATE SET
        theme_mode = COALESCE(excluded.theme_mode, settings.theme_mode),
        dark_mode = COALESCE(excluded.dark_mode, settings.dark_mode),
        notifications_enabled = COALESCE(excluded.notifications_enabled, settings.notifications_enabled),
        chat_notifications = COALESCE(excluded.chat_notifications, settings.chat_notifications),
        update_notifications = COALESCE(excluded.update_notifications, settings.update_notifications),
        reminder_notifications = COALESCE(excluded.reminder_notifications, settings.reminder_notifications),
        language = COALESCE(excluded.language, settings.language),
        biometric_lock = COALESCE(excluded.biometric_lock, settings.biometric_lock),
        app_version = COALESCE(excluded.app_version, settings.app_version),
        updated_at = COALESCE(excluded.updated_at, settings.updated_at);
    )sql";

    // derive values (use json or default to NULL so COALESCE keeps existing)
    std::string theme_mode = j.contains("theme_mode") ? j["theme_mode"].get<std::string>() : "";
    int dark_mode = j.contains("dark_mode") ? (j["dark_mode"].get<bool>() ? 1 : 0) : -1;
    int notifications_enabled = j.contains("notifications_enabled") ? (j["notifications_enabled"].get<bool>() ? 1 : 0) : -1;
    int chat_notifications = j.contains("chat_notifications") ? (j["chat_notifications"].get<bool>() ? 1 : 0) : -1;
    int update_notifications = j.contains("update_notifications") ? (j["update_notifications"].get<bool>() ? 1 : 0) : -1;
    int reminder_notifications = j.contains("reminder_notifications") ? (j["reminder_notifications"].get<bool>() ? 1 : 0) : -1;
    std::string language = j.contains("language") ? j["language"].get<std::string>() : "";
    int biometric_lock = j.contains("biometric_lock") ? (j["biometric_lock"].get<bool>() ? 1 : 0) : -1;
    std::string app_version = j.contains("app_version") ? j["app_version"].get<std::string>() : "";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        // Bind in order as in VALUES
        sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

        if (!theme_mode.empty()) sqlite3_bind_text(stmt, 2, theme_mode.c_str(), -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(stmt, 2);

        if (dark_mode != -1) sqlite3_bind_int(stmt, 3, dark_mode); else sqlite3_bind_null(stmt, 3);
        if (notifications_enabled != -1) sqlite3_bind_int(stmt, 4, notifications_enabled); else sqlite3_bind_null(stmt, 4);
        if (chat_notifications != -1) sqlite3_bind_int(stmt, 5, chat_notifications); else sqlite3_bind_null(stmt, 5);
        if (update_notifications != -1) sqlite3_bind_int(stmt, 6, update_notifications); else sqlite3_bind_null(stmt, 6);
        if (reminder_notifications != -1) sqlite3_bind_int(stmt, 7, reminder_notifications); else sqlite3_bind_null(stmt, 7);
        if (!language.empty()) sqlite3_bind_text(stmt, 8, language.c_str(), -1, SQLITE_TRANSIENT); else sqlite3_bind_null(stmt, 8);
        if (biometric_lock != -1) sqlite3_bind_int(stmt, 9, biometric_lock); else sqlite3_bind_null(stmt, 9);
        if (!app_version.empty()) sqlite3_bind_text(stmt, 10, app_version.c_str(), -1, SQLITE_TRANSIENT); else sqlite3_bind_null(stmt, 10);
        sqlite3_bind_text(stmt, 11, iso_now().c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);

    exec_sql(db, "COMMIT;");
    sqlite3_close(db);
    return true;
}

// Append chat message for user
bool append_chat_message(const std::string& user_id, const std::string& role, const std::string& message) {
    std::lock_guard<std::mutex> lock(db_mutex);
    ensure_user_exists(user_id);
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) return false;

    std::string sql = "INSERT INTO chat_history(user_id, role, message, created_at) VALUES(?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, iso_now().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

// Clear chat history for a user
bool clear_chat_history(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) return false;
    std::string sql = "DELETE FROM chat_history WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

// Export chat history + settings as JSON
json export_user_data(const std::string& user_id) {
    json out;
    out["exported_at"] = iso_now();
    out["settings"] = get_user_settings(user_id);

    // fetch chat_history rows
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) == SQLITE_OK) {
        std::string sql = "SELECT role, message, created_at FROM chat_history WHERE user_id = ? ORDER BY id ASC;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
            json arr = json::array();
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                json m;
                m["role"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                m["message"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                m["created_at"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                arr.push_back(m);
            }
            out["chat_history"] = arr;
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }

    return out;
}

// Import user data (merge: if replace==true, wipe chat_history first)
bool import_user_data(const std::string& user_id, const json& payload, bool replace = false) {
    std::lock_guard<std::mutex> lock(db_mutex);
    ensure_user_exists(user_id);
    sqlite3* db = nullptr;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) return false;

    exec_sql(db, "BEGIN TRANSACTION;");
    if (payload.contains("settings")) {
        upsert_settings(user_id, payload["settings"]);
    }
    if (payload.contains("chat_history")) {
        if (replace) {
            // wipe first
            std::string sql = "DELETE FROM chat_history WHERE user_id = ?;";
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
        }
        // insert each message
        std::string ins = "INSERT INTO chat_history(user_id, role, message, created_at) VALUES(?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, ins.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            for (auto& m : payload["chat_history"]) {
                std::string role = m.value("role", "user");
                std::string message = m.value("message", "");
                std::string created_at = m.value("created_at", iso_now());
                sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 4, created_at.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
        sqlite3_finalize(stmt);
    }

    exec_sql(db, "COMMIT;");
    sqlite3_close(db);
    return true;
}

// Basic API key check (replace with proper auth in prod)
bool authorize(const Request& req, Response& res) {
    auto it = req.headers.find(API_KEY_HEADER);
    if (it == req.headers.end() || it->second != VALID_API_KEY) {
        res.status = 401;
        res.set_content(R"({"error":"unauthorized"})", "application/json");
        return false;
    }
    return true;
}

// --- Server and routes --- //
int main() {
    init_db();
    Server svr;

    // Middleware: basic auth
    svr.set_pre_routing_handler([](const Request &req, Response &res) {
        // allow health & import if needed without key? require key globally
        if (req.path == "/health") return true;
        auto it = req.headers.find(API_KEY_HEADER);
        if (it == req.headers.end() || it->second != VALID_API_KEY) {
            res.status = 401;
            res.set_content(R"({"error":"unauthorized"})", "application/json");
            return false;
        }
        return true;
    });

    // Health
    svr.Get("/health", [](const Request& req, Response& res) {
        json out = {
                {"status", "ok"},
                {"time", iso_now()}
        };
        res.set_content(out.dump(), "application/json");
    });

    // GET settings
    svr.Get("/settings", [](const Request& req, Response& res) {
        auto user_it = req.get_param_value("user_id");
        if (user_it.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"user_id required"})", "application/json");
            return;
        }
        json s = get_user_settings(user_it);
        res.set_content(s.dump(), "application/json");
    });

    // POST settings (partial allowed)
    svr.Post("/settings", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id")) {
                res.status = 400;
                res.set_content(R"({"error":"user_id required"})", "application/json");
                return;
            }
            std::string user_id = j["user_id"];
            json payload = j.value("settings", j); // allow passing settings directly or inside "settings"
            // remove user_id if present in payload
            payload.erase("user_id");
            upsert_settings(user_id, payload);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err = {{"error","invalid json"}, {"detail", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    // POST profile update
    svr.Post("/profile", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id")) {
                res.status = 400;
                res.set_content(R"({"error":"user_id required"})", "application/json");
                return;
            }
            std::string user_id = j["user_id"];
            json payload;
            if (j.contains("name")) payload["name"] = j["name"];
            if (j.contains("email")) payload["email"] = j["email"];
            if (j.contains("avatar_url")) payload["avatar_url"] = j["avatar_url"];
            upsert_settings(user_id, payload);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid request"})", "application/json");
        }
    });

    // POST notifications (granular)
    svr.Post("/notifications", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id")) { res.status = 400; res.set_content(R"({"error":"user_id required"})", "application/json"); return; }
            std::string user_id = j["user_id"];
            json payload;
            if (j.contains("notifications_enabled")) payload["notifications_enabled"] = j["notifications_enabled"];
            if (j.contains("chat_notifications")) payload["chat_notifications"] = j["chat_notifications"];
            if (j.contains("update_notifications")) payload["update_notifications"] = j["update_notifications"];
            if (j.contains("reminder_notifications")) payload["reminder_notifications"] = j["reminder_notifications"];
            upsert_settings(user_id, payload);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error":"invalid request"})", "application/json"); }
    });

    // POST theme
    svr.Post("/theme", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id") || !j.contains("theme_mode")) { res.status = 400; res.set_content(R"({"error":"user_id and theme_mode required"})", "application/json"); return; }
            std::string user_id = j["user_id"];
            std::string mode = j["theme_mode"];
            json payload;
            payload["theme_mode"] = mode;
            payload["dark_mode"] = (mode == "Dark");
            upsert_settings(user_id, payload);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error":"invalid request"})", "application/json"); }
    });

    // POST security biometric lock
    svr.Post("/security/biometric", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id") || !j.contains("enabled")) { res.status = 400; res.set_content(R"({"error":"user_id and enabled required"})", "application/json"); return; }
            std::string user_id = j["user_id"];
            bool enabled = j["enabled"];
            json payload;
            payload["biometric_lock"] = enabled;
            upsert_settings(user_id, payload);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error":"invalid request"})", "application/json"); }
    });

    // POST append chat message
    svr.Post("/history", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id") || !j.contains("role") || !j.contains("message")) {
                res.status = 400;
                res.set_content(R"({"error":"user_id, role, message required"})", "application/json");
                return;
            }
            append_chat_message(j["user_id"], j["role"], j["message"]);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error":"invalid request"})", "application/json"); }
    });

    // POST clear history
    svr.Post("/history/clear", [](const Request& req, Response& res) {
        try {
            json j = json::parse(req.body);
            if (!j.contains("user_id")) { res.status = 400; res.set_content(R"({"error":"user_id required"})", "application/json"); return; }
            clear_chat_history(j["user_id"]);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error":"invalid request"})", "application/json"); }
    });

    // GET export
    svr.Get("/history/export", [](const Request& req, Response& res) {
        auto user_id = req.get_param_value("user_id");
        if (user_id.empty()) { res.status = 400; res.set_content(R"({"error":"user_id required"})", "application/json"); return; }
        json out = export_user_data(user_id);
        res.set_content(out.dump(2), "application/json");
    });

    // POST import (replace param optional: ?replace=true)
    svr.Post("/history/import", [](const Request& req, Response& res) {
        try {
            bool replace = false;
            auto q = req.get_param_value("replace");
            if (!q.empty() && (q == "1" || q == "true")) replace = true;
            json j = json::parse(req.body);
            if (!j.contains("user_id")) { res.status = 400; res.set_content(R"({"error":"user_id required"})", "application/json"); return; }
            std::string user_id = j["user_id"];
            import_user_data(user_id, j, replace);
            res.set_content(R"({"ok":true})", "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            json err = {{"error","invalid request"}, {"detail", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    // GET history (simple list)
    svr.Get("/history", [](const Request& req, Response& res) {
        auto user_id = req.get_param_value("user_id");
        if (user_id.empty()) { res.status = 400; res.set_content(R"({"error":"user_id required"})", "application/json"); return; }
        // reuse export_user_data but return only chat_history
        json data = export_user_data(user_id);
        res.set_content(data["chat_history"].dump(2), "application/json");
    });

    // Start server
    std::cout << "Starting settings server on http://0.0.0.0:8080\n";
    svr.listen("0.0.0.0", 8080);

    return 0;
}
