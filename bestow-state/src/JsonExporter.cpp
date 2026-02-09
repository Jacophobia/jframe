// bestow-state/src/JsonExporter.cpp
// JSON export/import for debugging and manual save editing

module;

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

module bestow.state.impl;

using json = nlohmann::json;

namespace bestow {

Result<void, StateError> JsonExporter::exportToJson(
    StateDatabase& db, StateSlot slot, const std::string& path) {

    auto meta = db.readMetadata(slot);
    if (!meta) {
        return std::unexpected(StateError::SlotNotFound);
    }

    auto data = db.readSlot(slot);

    json doc;
    doc["formatVersion"] = meta->formatVersion;

    json metaJson;
    metaJson["name"] = meta->name;
    metaJson["gameVersion"] = meta->gameVersion;
    metaJson["playtimeSeconds"] = meta->playtimeSeconds;
    metaJson["completionPercentage"] = meta->completionPercentage;
    metaJson["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        meta->timestamp.time_since_epoch()).count();
    if (meta->levelName) {
        metaJson["levelName"] = *meta->levelName;
    }
    doc["metadata"] = metaJson;

    json dataJson;
    for (const auto& [key, entry] : data) {
        switch (entry.type) {
            case ValueType::Number:
                dataJson[key] = entry.numValue;
                break;
            case ValueType::String:
                dataJson[key] = entry.strValue;
                break;
            case ValueType::Bool:
                dataJson[key] = (entry.numValue != 0.0);
                break;
            case ValueType::Json:
                dataJson[key] = json::parse(entry.strValue, nullptr, false);
                break;
            case ValueType::Null:
                dataJson[key] = nullptr;
                break;
        }
    }
    doc["data"] = dataJson;

    std::ofstream file(path);
    if (!file) {
        return std::unexpected(StateError::IOError);
    }
    file << doc.dump(2);
    file.close();

    spdlog::info("[State] Exported slot {} to {}", slot, path);
    return {};
}

Result<void, StateError> JsonExporter::importFromJson(
    StateDatabase& db, StateSlot slot, const std::string& path) {

    std::ifstream file(path);
    if (!file) {
        return std::unexpected(StateError::IOError);
    }

    json doc;
    try {
        file >> doc;
    } catch (const json::parse_error& e) {
        spdlog::error("[State] JSON parse error: {}", e.what());
        return std::unexpected(StateError::JsonParseError);
    }

    // Build metadata
    StateMetadata meta;
    meta.slot = slot;
    meta.formatVersion = doc.value("formatVersion", 1);

    if (doc.contains("metadata")) {
        auto& m = doc["metadata"];
        meta.name = m.value("name", "");
        meta.gameVersion = m.value("gameVersion", "1.0.0");
        meta.playtimeSeconds = m.value("playtimeSeconds", 0ULL);
        meta.completionPercentage = m.value("completionPercentage", 0.0f);
        if (m.contains("timestamp")) {
            meta.timestamp = std::chrono::system_clock::time_point(
                std::chrono::seconds(m["timestamp"].get<std::int64_t>()));
        } else {
            meta.timestamp = std::chrono::system_clock::now();
        }
        if (m.contains("levelName")) {
            meta.levelName = m["levelName"].get<std::string>();
        }
    } else {
        meta.timestamp = std::chrono::system_clock::now();
    }

    // Build data map
    std::unordered_map<std::string, CacheEntry> data;
    if (doc.contains("data")) {
        for (auto& [key, val] : doc["data"].items()) {
            CacheEntry entry;
            if (val.is_number()) {
                entry.type = ValueType::Number;
                entry.numValue = val.get<double>();
            } else if (val.is_string()) {
                entry.type = ValueType::String;
                entry.strValue = val.get<std::string>();
            } else if (val.is_boolean()) {
                entry.type = ValueType::Bool;
                entry.numValue = val.get<bool>() ? 1.0 : 0.0;
            } else if (val.is_object() || val.is_array()) {
                entry.type = ValueType::Json;
                entry.strValue = val.dump();
            } else {
                entry.type = ValueType::Null;
            }
            data[key] = std::move(entry);
        }
    }

    bool ok = db.writeSlot(slot, data, meta);
    if (!ok) {
        return std::unexpected(StateError::DatabaseError);
    }

    spdlog::info("[State] Imported slot {} from {}", slot, path);
    return {};
}

}  // namespace bestow
