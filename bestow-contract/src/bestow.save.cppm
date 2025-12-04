// bestow-contract/src/bestow.save.cppm
// Save system interface

module;

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

export module bestow.save;

import bestow.types;

export namespace bestow {

// Abstract archive interfaces for serialization
// Concrete implementations provided by bestow.save.impl
class ISaveArchive {
public:
    virtual ~ISaveArchive() = default;

    // Write a named value to the archive
    virtual void writeInt(const std::string& key, int value) = 0;
    virtual void writeFloat(const std::string& key, float value) = 0;
    virtual void writeDouble(const std::string& key, double value) = 0;
    virtual void writeString(const std::string& key, const std::string& value) = 0;
    virtual void writeBool(const std::string& key, bool value) = 0;
    virtual void writeBytes(const std::string& key, const std::vector<std::uint8_t>& value) = 0;
};

class ILoadArchive {
public:
    virtual ~ILoadArchive() = default;

    // Read a named value from the archive
    virtual int readInt(const std::string& key) const = 0;
    virtual float readFloat(const std::string& key) const = 0;
    virtual double readDouble(const std::string& key) const = 0;
    virtual std::string readString(const std::string& key) const = 0;
    virtual bool readBool(const std::string& key) const = 0;
    virtual std::vector<std::uint8_t> readBytes(const std::string& key) const = 0;
};

// Type aliases for compatibility
using SaveArchive = ISaveArchive;
using LoadArchive = ILoadArchive;

class ISaveable {
public:
    virtual ~ISaveable() = default;
    virtual std::string getSaveKey() const = 0;
    virtual void serialize(ISaveArchive& archive) const = 0;
    virtual void deserialize(const ILoadArchive& archive) = 0;
};

struct SaveMetadata {
    SaveSlot slot;
    std::string saveName;
    std::chrono::system_clock::time_point timestamp;
    std::string gameVersion;
    std::uint64_t playtimeSeconds = 0;
    float completionPercentage = 0.0f;
    std::optional<std::string> levelName;
    bool hasScreenshot = false;
};

class ISaveSystem {
public:
    virtual ~ISaveSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Saveable Registration
    //======================================================================

    virtual void registerSaveable(ISaveable* saveable) = 0;
    virtual void unregisterSaveable(ISaveable* saveable) = 0;

    //======================================================================
    // Save/Load Operations
    //======================================================================

    virtual Result<void, SaveError> save(SaveSlot slot,
                                          const std::string& saveName) = 0;
    virtual Result<void, SaveError> load(SaveSlot slot) = 0;
    virtual bool deleteSave(SaveSlot slot) = 0;

    //======================================================================
    // Quick Save/Load
    //======================================================================

    virtual void quickSave() = 0;
    virtual void quickLoad() = 0;

    //======================================================================
    // Auto-Save
    //======================================================================

    virtual void autoSave() = 0;
    virtual void enableAutoSave(std::chrono::seconds interval) = 0;
    virtual void disableAutoSave() = 0;

    //======================================================================
    // Metadata Queries
    //======================================================================

    virtual std::vector<SaveMetadata> getAllSaveMetadata() const = 0;
    virtual std::optional<SaveMetadata> getSaveMetadata(SaveSlot slot) const = 0;
    virtual bool saveExists(SaveSlot slot) const = 0;

    //======================================================================
    // Profile Management
    //======================================================================

    virtual void setActiveProfile(const std::string& profileId) = 0;
    virtual std::string getActiveProfile() const = 0;
    virtual std::vector<std::string> getProfiles() const = 0;
};

}  // namespace bestow
