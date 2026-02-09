// bestow-state/src/StateDatabase.cpp
// SQLite wrapper implementation with PIMPL

module;

#include <sqlite3.h>
#include <spdlog/spdlog.h>

module bestow.state.impl;

namespace bestow {

//==========================================================================
// PIMPL implementation
//==========================================================================

struct StateDatabaseImpl {
    sqlite3* db = nullptr;

    // Prepared statements for performance
    sqlite3_stmt* stmtInsertSlot = nullptr;
    sqlite3_stmt* stmtInsertData = nullptr;
    sqlite3_stmt* stmtDeleteData = nullptr;
    sqlite3_stmt* stmtDeleteSlot = nullptr;
    sqlite3_stmt* stmtReadData = nullptr;
    sqlite3_stmt* stmtReadMeta = nullptr;
    sqlite3_stmt* stmtSlotExists = nullptr;

    ~StateDatabaseImpl() {
        finalize();
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    void finalize() {
        auto fin = [](sqlite3_stmt*& s) {
            if (s) { sqlite3_finalize(s); s = nullptr; }
        };
        fin(stmtInsertSlot);
        fin(stmtInsertData);
        fin(stmtDeleteData);
        fin(stmtDeleteSlot);
        fin(stmtReadData);
        fin(stmtReadMeta);
        fin(stmtSlotExists);
    }
};

//==========================================================================
// StateDatabase
//==========================================================================

StateDatabase::StateDatabase() : impl_(std::make_unique<StateDatabaseImpl>()) {}

StateDatabase::~StateDatabase() = default;

StateDatabase::StateDatabase(StateDatabase&&) noexcept = default;
StateDatabase& StateDatabase::operator=(StateDatabase&&) noexcept = default;

bool StateDatabase::open(const std::filesystem::path& dbPath) {
    if (impl_->db) {
        close();
    }

    // Create parent directories
    std::filesystem::create_directories(dbPath.parent_path());

    int rc = sqlite3_open(dbPath.string().c_str(), &impl_->db);
    if (rc != SQLITE_OK) {
        spdlog::error("[State] Failed to open database {}: {}", dbPath.string(),
                      sqlite3_errmsg(impl_->db));
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
        return false;
    }

    // Enable WAL mode for crash resilience
    char* errMsg = nullptr;
    sqlite3_exec(impl_->db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }

    // Enable foreign keys
    sqlite3_exec(impl_->db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }

    // Create schema
    const char* schema = R"SQL(
        CREATE TABLE IF NOT EXISTS slots (
            slot            INTEGER PRIMARY KEY,
            name            TEXT NOT NULL DEFAULT '',
            timestamp       INTEGER NOT NULL,
            game_version    TEXT NOT NULL DEFAULT '1.0.0',
            playtime_seconds INTEGER NOT NULL DEFAULT 0,
            completion_pct  REAL NOT NULL DEFAULT 0.0,
            level_name      TEXT,
            format_version  INTEGER NOT NULL DEFAULT 1
        );

        CREATE TABLE IF NOT EXISTS data (
            slot        INTEGER NOT NULL,
            key         TEXT NOT NULL,
            value_type  INTEGER NOT NULL,
            value_num   REAL,
            value_str   TEXT,
            PRIMARY KEY (slot, key),
            FOREIGN KEY (slot) REFERENCES slots(slot) ON DELETE CASCADE
        );
    )SQL";

    rc = sqlite3_exec(impl_->db, schema, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        spdlog::error("[State] Failed to create schema: {}", errMsg ? errMsg : "unknown");
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    // Prepare statements
    auto prep = [&](const char* sql, sqlite3_stmt*& stmt) -> bool {
        int r = sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr);
        if (r != SQLITE_OK) {
            spdlog::error("[State] Failed to prepare statement: {}", sqlite3_errmsg(impl_->db));
            return false;
        }
        return true;
    };

    prep("INSERT OR REPLACE INTO slots (slot, name, timestamp, game_version, playtime_seconds, completion_pct, level_name, format_version) VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
         impl_->stmtInsertSlot);
    prep("INSERT OR REPLACE INTO data (slot, key, value_type, value_num, value_str) VALUES (?, ?, ?, ?, ?);",
         impl_->stmtInsertData);
    prep("DELETE FROM data WHERE slot = ?;",
         impl_->stmtDeleteData);
    prep("DELETE FROM slots WHERE slot = ?;",
         impl_->stmtDeleteSlot);
    prep("SELECT key, value_type, value_num, value_str FROM data WHERE slot = ?;",
         impl_->stmtReadData);
    prep("SELECT slot, name, timestamp, game_version, playtime_seconds, completion_pct, level_name, format_version FROM slots WHERE slot = ?;",
         impl_->stmtReadMeta);
    prep("SELECT 1 FROM slots WHERE slot = ? LIMIT 1;",
         impl_->stmtSlotExists);

    spdlog::debug("[State] Database opened: {}", dbPath.string());
    return true;
}

void StateDatabase::close() {
    impl_->finalize();
    if (impl_->db) {
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
    }
}

bool StateDatabase::isOpen() const {
    return impl_->db != nullptr;
}

bool StateDatabase::writeSlot(StateSlot slot,
                              const std::unordered_map<std::string, CacheEntry>& data,
                              const StateMetadata& metadata) {
    if (!impl_->db) return false;

    char* errMsg = nullptr;
    sqlite3_exec(impl_->db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (errMsg) { sqlite3_free(errMsg); errMsg = nullptr; }

    // Delete old data for this slot
    sqlite3_reset(impl_->stmtDeleteData);
    sqlite3_bind_int(impl_->stmtDeleteData, 1, static_cast<int>(slot));
    sqlite3_step(impl_->stmtDeleteData);

    // Write metadata
    sqlite3_reset(impl_->stmtInsertSlot);
    sqlite3_bind_int(impl_->stmtInsertSlot, 1, static_cast<int>(slot));
    sqlite3_bind_text(impl_->stmtInsertSlot, 2, metadata.name.c_str(), -1, SQLITE_TRANSIENT);
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        metadata.timestamp.time_since_epoch()).count();
    sqlite3_bind_int64(impl_->stmtInsertSlot, 3, ts);
    sqlite3_bind_text(impl_->stmtInsertSlot, 4, metadata.gameVersion.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(impl_->stmtInsertSlot, 5, static_cast<sqlite3_int64>(metadata.playtimeSeconds));
    sqlite3_bind_double(impl_->stmtInsertSlot, 6, metadata.completionPercentage);
    if (metadata.levelName) {
        sqlite3_bind_text(impl_->stmtInsertSlot, 7, metadata.levelName->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(impl_->stmtInsertSlot, 7);
    }
    sqlite3_bind_int(impl_->stmtInsertSlot, 8, metadata.formatVersion);

    int rc = sqlite3_step(impl_->stmtInsertSlot);
    if (rc != SQLITE_DONE) {
        spdlog::error("[State] Failed to write slot metadata: {}", sqlite3_errmsg(impl_->db));
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    // Write all key-value pairs
    for (const auto& [key, entry] : data) {
        sqlite3_reset(impl_->stmtInsertData);
        sqlite3_bind_int(impl_->stmtInsertData, 1, static_cast<int>(slot));
        sqlite3_bind_text(impl_->stmtInsertData, 2, key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(impl_->stmtInsertData, 3, static_cast<int>(entry.type));

        switch (entry.type) {
            case ValueType::Number:
                sqlite3_bind_double(impl_->stmtInsertData, 4, entry.numValue);
                sqlite3_bind_null(impl_->stmtInsertData, 5);
                break;
            case ValueType::Bool:
                sqlite3_bind_double(impl_->stmtInsertData, 4, entry.numValue);
                sqlite3_bind_null(impl_->stmtInsertData, 5);
                break;
            case ValueType::String:
            case ValueType::Json:
                sqlite3_bind_null(impl_->stmtInsertData, 4);
                sqlite3_bind_text(impl_->stmtInsertData, 5, entry.strValue.c_str(), -1, SQLITE_TRANSIENT);
                break;
            case ValueType::Null:
                sqlite3_bind_null(impl_->stmtInsertData, 4);
                sqlite3_bind_null(impl_->stmtInsertData, 5);
                break;
        }

        rc = sqlite3_step(impl_->stmtInsertData);
        if (rc != SQLITE_DONE) {
            spdlog::error("[State] Failed to write data key '{}': {}", key, sqlite3_errmsg(impl_->db));
            sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }

    sqlite3_exec(impl_->db, "COMMIT;", nullptr, nullptr, &errMsg);
    if (errMsg) {
        spdlog::error("[State] Failed to commit: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

std::unordered_map<std::string, CacheEntry> StateDatabase::readSlot(StateSlot slot) {
    std::unordered_map<std::string, CacheEntry> result;
    if (!impl_->db || !impl_->stmtReadData) return result;

    sqlite3_reset(impl_->stmtReadData);
    sqlite3_bind_int(impl_->stmtReadData, 1, static_cast<int>(slot));

    while (sqlite3_step(impl_->stmtReadData) == SQLITE_ROW) {
        const char* key = reinterpret_cast<const char*>(sqlite3_column_text(impl_->stmtReadData, 0));
        int valueType = sqlite3_column_int(impl_->stmtReadData, 1);

        CacheEntry entry;
        entry.type = static_cast<ValueType>(valueType);

        switch (entry.type) {
            case ValueType::Number:
            case ValueType::Bool:
                entry.numValue = sqlite3_column_double(impl_->stmtReadData, 2);
                break;
            case ValueType::String:
            case ValueType::Json: {
                const char* str = reinterpret_cast<const char*>(sqlite3_column_text(impl_->stmtReadData, 3));
                if (str) entry.strValue = str;
                break;
            }
            case ValueType::Null:
                break;
        }

        if (key) result[key] = std::move(entry);
    }

    return result;
}

std::optional<StateMetadata> StateDatabase::readMetadata(StateSlot slot) {
    if (!impl_->db || !impl_->stmtReadMeta) return std::nullopt;

    sqlite3_reset(impl_->stmtReadMeta);
    sqlite3_bind_int(impl_->stmtReadMeta, 1, static_cast<int>(slot));

    if (sqlite3_step(impl_->stmtReadMeta) != SQLITE_ROW) {
        return std::nullopt;
    }

    StateMetadata meta;
    meta.slot = static_cast<StateSlot>(sqlite3_column_int(impl_->stmtReadMeta, 0));

    const char* name = reinterpret_cast<const char*>(sqlite3_column_text(impl_->stmtReadMeta, 1));
    if (name) meta.name = name;

    auto ts = sqlite3_column_int64(impl_->stmtReadMeta, 2);
    meta.timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(ts));

    const char* version = reinterpret_cast<const char*>(sqlite3_column_text(impl_->stmtReadMeta, 3));
    if (version) meta.gameVersion = version;

    meta.playtimeSeconds = static_cast<std::uint64_t>(sqlite3_column_int64(impl_->stmtReadMeta, 4));
    meta.completionPercentage = static_cast<float>(sqlite3_column_double(impl_->stmtReadMeta, 5));

    const char* level = reinterpret_cast<const char*>(sqlite3_column_text(impl_->stmtReadMeta, 6));
    if (level) meta.levelName = std::string(level);

    meta.formatVersion = sqlite3_column_int(impl_->stmtReadMeta, 7);

    return meta;
}

std::vector<StateMetadata> StateDatabase::readAllMetadata() {
    std::vector<StateMetadata> result;
    if (!impl_->db) return result;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT slot, name, timestamp, game_version, playtime_seconds, completion_pct, level_name, format_version FROM slots ORDER BY timestamp DESC;";
    if (sqlite3_prepare_v2(impl_->db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        StateMetadata meta;
        meta.slot = static_cast<StateSlot>(sqlite3_column_int(stmt, 0));

        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name) meta.name = name;

        auto ts = sqlite3_column_int64(stmt, 2);
        meta.timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(ts));

        const char* version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (version) meta.gameVersion = version;

        meta.playtimeSeconds = static_cast<std::uint64_t>(sqlite3_column_int64(stmt, 4));
        meta.completionPercentage = static_cast<float>(sqlite3_column_double(stmt, 5));

        const char* level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        if (level) meta.levelName = std::string(level);

        meta.formatVersion = sqlite3_column_int(stmt, 7);

        result.push_back(std::move(meta));
    }

    sqlite3_finalize(stmt);
    return result;
}

bool StateDatabase::deleteSlot(StateSlot slot) {
    if (!impl_->db) return false;

    // Foreign key CASCADE handles data table cleanup
    sqlite3_reset(impl_->stmtDeleteSlot);
    sqlite3_bind_int(impl_->stmtDeleteSlot, 1, static_cast<int>(slot));
    int rc = sqlite3_step(impl_->stmtDeleteSlot);
    return rc == SQLITE_DONE && sqlite3_changes(impl_->db) > 0;
}

bool StateDatabase::slotExists(StateSlot slot) {
    if (!impl_->db || !impl_->stmtSlotExists) return false;

    sqlite3_reset(impl_->stmtSlotExists);
    sqlite3_bind_int(impl_->stmtSlotExists, 1, static_cast<int>(slot));
    return sqlite3_step(impl_->stmtSlotExists) == SQLITE_ROW;
}

}  // namespace bestow
