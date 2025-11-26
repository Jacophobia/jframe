// jframe-save/src/SaveSystem.cpp
// Save system implementation

module;

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

module jframe.save.impl;

using json = nlohmann::json;

namespace jframe {

void SaveSystem::update(DeltaTime dt) {
    if (autoSaveEnabled_) {
        autoSaveTimer_ += dt;
        if (autoSaveTimer_ >= autoSaveInterval_.count()) {
            autoSave();
            autoSaveTimer_ = 0.0f;
        }
    }
}

void SaveSystem::registerSaveable(ISaveable* saveable) {
    saveables_.push_back(saveable);
}

void SaveSystem::unregisterSaveable(ISaveable* saveable) {
    saveables_.erase(
        std::remove(saveables_.begin(), saveables_.end(), saveable),
        saveables_.end());
}

Result<void, SaveError> SaveSystem::save(SaveSlot slot, const std::string& saveName) {
    try {
        auto savePath = getSavePath(slot);
        auto metaPath = getMetadataPath(slot);

        // Create save directory if it doesn't exist
        std::filesystem::create_directories(savePath.parent_path());

        // Open save file for binary writing
        std::ofstream saveFile(savePath, std::ios::binary);
        if (!saveFile) {
            return std::unexpected(SaveError::IOError);
        }

        // Write file header
        constexpr std::uint32_t MAGIC_NUMBER = 0x4A465356; // "JFSV" (JFrame Save)
        constexpr std::uint32_t VERSION = 1;
        saveFile.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(MAGIC_NUMBER));
        saveFile.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));

        // Create save archive and serialize all registered saveables
        CerealSaveArchive archive(saveFile);

        // Write number of saveables directly to cereal archive
        std::uint32_t saveableCount = static_cast<std::uint32_t>(saveables_.size());
        archive.getArchive()(saveableCount);

        // Serialize each saveable with its key
        for (auto* saveable : saveables_) {
            std::string key = saveable->getSaveKey();
            archive.getArchive()(key);
            saveable->serialize(archive);
        }

        saveFile.close();

        // Write metadata JSON file
        auto now = std::chrono::system_clock::now();
        json metadata;
        metadata["slot"] = slot;
        metadata["saveName"] = saveName;
        metadata["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        metadata["gameVersion"] = "0.1.0"; // TODO: Get from game config
        metadata["playtimeSeconds"] = 0;    // TODO: Track playtime
        metadata["completionPercentage"] = 0.0f; // TODO: Track completion

        std::ofstream metaFile(metaPath);
        if (!metaFile) {
            return std::unexpected(SaveError::IOError);
        }

        metaFile << metadata.dump(2);
        metaFile.close();

        return {};

    } catch (const std::exception&) {
        return std::unexpected(SaveError::SerializationError);
    }
}

Result<void, SaveError> SaveSystem::load(SaveSlot slot) {
    try {
        auto savePath = getSavePath(slot);

        // Check if save file exists
        if (!std::filesystem::exists(savePath)) {
            return std::unexpected(SaveError::FileNotFound);
        }

        // Open save file for binary reading
        std::ifstream saveFile(savePath, std::ios::binary);
        if (!saveFile) {
            return std::unexpected(SaveError::IOError);
        }

        // Read and verify file header
        std::uint32_t magicNumber = 0;
        std::uint32_t version = 0;
        saveFile.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));
        saveFile.read(reinterpret_cast<char*>(&version), sizeof(version));

        constexpr std::uint32_t EXPECTED_MAGIC = 0x4A465356; // "JFSV"
        if (magicNumber != EXPECTED_MAGIC) {
            return std::unexpected(SaveError::CorruptedFile);
        }

        constexpr std::uint32_t CURRENT_VERSION = 1;
        if (version != CURRENT_VERSION) {
            return std::unexpected(SaveError::VersionMismatch);
        }

        // Create load archive
        CerealLoadArchive archive(saveFile);

        // Read number of saveables from cereal archive
        std::uint32_t saveableCount = 0;
        archive.getArchive()(saveableCount);

        // Create a map of saveables by their keys for fast lookup
        std::map<std::string, ISaveable*> saveableMap;
        for (auto* saveable : saveables_) {
            saveableMap[saveable->getSaveKey()] = saveable;
        }

        // Deserialize each saved component
        for (std::uint32_t i = 0; i < saveableCount; ++i) {
            std::string key;
            archive.getArchive()(key);

            // Find the corresponding saveable
            auto it = saveableMap.find(key);
            if (it != saveableMap.end()) {
                it->second->deserialize(archive);
            } else {
                // TODO: Skip unknown saveable data
                // For now, this will fail if save has data for unregistered saveables
                return std::unexpected(SaveError::SerializationError);
            }
        }

        saveFile.close();
        return {};

    } catch (const std::exception&) {
        return std::unexpected(SaveError::SerializationError);
    }
}

bool SaveSystem::deleteSave(SaveSlot slot) {
    auto path = getSavePath(slot);
    return std::filesystem::remove(path);
}

void SaveSystem::quickSave() {
    save(SaveSlots::QuickSave, "Quick Save");
}

void SaveSystem::quickLoad() {
    load(SaveSlots::QuickSave);
}

void SaveSystem::autoSave() {
    save(SaveSlots::AutoSave, "Auto Save");
}

void SaveSystem::enableAutoSave(std::chrono::seconds interval) {
    autoSaveInterval_ = interval;
    autoSaveEnabled_ = true;
    autoSaveTimer_ = 0.0f;
}

void SaveSystem::disableAutoSave() {
    autoSaveEnabled_ = false;
}

std::vector<SaveMetadata> SaveSystem::getAllSaveMetadata() const {
    std::vector<SaveMetadata> result;

    auto profilePath = savesDirectory_ / activeProfile_;

    // Check if profile directory exists
    if (!std::filesystem::exists(profilePath)) {
        return result;
    }

    // Scan for .meta files
    for (const auto& entry : std::filesystem::directory_iterator(profilePath)) {
        if (entry.path().extension() == ".meta") {
            // Extract slot number from filename (e.g., "save_1.meta" -> 1)
            std::string filename = entry.path().stem().string();
            if (filename.starts_with("save_")) {
                try {
                    SaveSlot slot = std::stoul(filename.substr(5));
                    auto metadata = getSaveMetadata(slot);
                    if (metadata) {
                        result.push_back(*metadata);
                    }
                } catch (const std::exception&) {
                    // Skip invalid filenames
                    continue;
                }
            }
        }
    }

    return result;
}

std::optional<SaveMetadata> SaveSystem::getSaveMetadata(SaveSlot slot) const {
    if (!saveExists(slot)) return std::nullopt;

    auto metaPath = getMetadataPath(slot);

    // Check if metadata file exists
    if (!std::filesystem::exists(metaPath)) {
        return std::nullopt;
    }

    try {
        std::ifstream metaFile(metaPath);
        if (!metaFile) {
            return std::nullopt;
        }

        json metadata;
        metaFile >> metadata;

        SaveMetadata result;
        result.slot = slot;
        result.saveName = metadata.value("saveName", "");
        result.gameVersion = metadata.value("gameVersion", "");
        result.playtimeSeconds = metadata.value("playtimeSeconds", 0);
        result.completionPercentage = metadata.value("completionPercentage", 0.0f);

        // Parse timestamp
        if (metadata.contains("timestamp")) {
            auto seconds = metadata["timestamp"].get<std::int64_t>();
            result.timestamp = std::chrono::system_clock::time_point(
                std::chrono::seconds(seconds));
        }

        // Optional fields
        if (metadata.contains("levelName")) {
            result.levelName = metadata["levelName"].get<std::string>();
        }

        result.hasScreenshot = metadata.value("hasScreenshot", false);

        return result;

    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool SaveSystem::saveExists(SaveSlot slot) const {
    return std::filesystem::exists(getSavePath(slot));
}

void SaveSystem::setActiveProfile(const std::string& profileId) {
    activeProfile_ = profileId;
}

std::string SaveSystem::getActiveProfile() const {
    return activeProfile_;
}

std::vector<std::string> SaveSystem::getProfiles() const {
    std::vector<std::string> profiles;

    // Check if saves directory exists
    if (!std::filesystem::exists(savesDirectory_)) {
        return profiles;
    }

    // Scan for subdirectories in saves directory - each is a profile
    for (const auto& entry : std::filesystem::directory_iterator(savesDirectory_)) {
        if (entry.is_directory()) {
            profiles.push_back(entry.path().filename().string());
        }
    }

    return profiles;
}

std::filesystem::path SaveSystem::getSavePath(SaveSlot slot) const {
    return savesDirectory_ / activeProfile_ / ("save_" + std::to_string(slot) + ".sav");
}

std::filesystem::path SaveSystem::getMetadataPath(SaveSlot slot) const {
    return savesDirectory_ / activeProfile_ / ("save_" + std::to_string(slot) + ".meta");
}

}  // namespace jframe
