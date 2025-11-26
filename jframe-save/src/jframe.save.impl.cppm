// jframe-save/src/jframe.save.impl.cppm
// Save system implementation

module;

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <nlohmann/json.hpp>

export module jframe.save.impl;

import std;
import jframe.save;
import jframe.types;

export namespace jframe {

// Concrete SaveArchive implementing ISaveArchive
class CerealSaveArchive : public ISaveArchive {
public:
    explicit CerealSaveArchive(std::ostream& stream)
        : archive_(stream) {}

    void writeInt(const std::string& key, int value) override {
        archive_(cereal::make_nvp(key.c_str(), value));
    }

    void writeFloat(const std::string& key, float value) override {
        archive_(cereal::make_nvp(key.c_str(), value));
    }

    void writeDouble(const std::string& key, double value) override {
        archive_(cereal::make_nvp(key.c_str(), value));
    }

    void writeString(const std::string& key, const std::string& value) override {
        archive_(cereal::make_nvp(key.c_str(), value));
    }

    void writeBool(const std::string& key, bool value) override {
        archive_(cereal::make_nvp(key.c_str(), value));
    }

    void writeBytes(const std::string& key, const std::vector<std::uint8_t>& value) override {
        archive_(cereal::make_nvp(key.c_str(), value));
    }

    // Direct access for internal use
    cereal::BinaryOutputArchive& getArchive() { return archive_; }

private:
    cereal::BinaryOutputArchive archive_;
};

// Concrete LoadArchive implementing ILoadArchive
class CerealLoadArchive : public ILoadArchive {
public:
    explicit CerealLoadArchive(std::istream& stream)
        : archive_(stream) {}

    int readInt(const std::string& key) const override {
        int value = 0;
        const_cast<cereal::BinaryInputArchive&>(archive_)(cereal::make_nvp(key.c_str(), value));
        return value;
    }

    float readFloat(const std::string& key) const override {
        float value = 0.0f;
        const_cast<cereal::BinaryInputArchive&>(archive_)(cereal::make_nvp(key.c_str(), value));
        return value;
    }

    double readDouble(const std::string& key) const override {
        double value = 0.0;
        const_cast<cereal::BinaryInputArchive&>(archive_)(cereal::make_nvp(key.c_str(), value));
        return value;
    }

    std::string readString(const std::string& key) const override {
        std::string value;
        const_cast<cereal::BinaryInputArchive&>(archive_)(cereal::make_nvp(key.c_str(), value));
        return value;
    }

    bool readBool(const std::string& key) const override {
        bool value = false;
        const_cast<cereal::BinaryInputArchive&>(archive_)(cereal::make_nvp(key.c_str(), value));
        return value;
    }

    std::vector<std::uint8_t> readBytes(const std::string& key) const override {
        std::vector<std::uint8_t> value;
        const_cast<cereal::BinaryInputArchive&>(archive_)(cereal::make_nvp(key.c_str(), value));
        return value;
    }

    // Direct access for internal use
    cereal::BinaryInputArchive& getArchive() const {
        return const_cast<cereal::BinaryInputArchive&>(archive_);
    }

private:
    cereal::BinaryInputArchive archive_;
};

class SaveSystem : public ISaveSystem {
public:
    SaveSystem() = default;
    ~SaveSystem() override = default;

    void update(DeltaTime dt) override;

    // Saveable registration
    void registerSaveable(ISaveable* saveable) override;
    void unregisterSaveable(ISaveable* saveable) override;

    // Save/Load operations
    Result<void, SaveError> save(SaveSlot slot, const std::string& saveName) override;
    Result<void, SaveError> load(SaveSlot slot) override;
    bool deleteSave(SaveSlot slot) override;

    // Quick save/load
    void quickSave() override;
    void quickLoad() override;

    // Auto-save
    void autoSave() override;
    void enableAutoSave(std::chrono::seconds interval) override;
    void disableAutoSave() override;

    // Metadata queries
    std::vector<SaveMetadata> getAllSaveMetadata() const override;
    std::optional<SaveMetadata> getSaveMetadata(SaveSlot slot) const override;
    bool saveExists(SaveSlot slot) const override;

    // Profile management
    void setActiveProfile(const std::string& profileId) override;
    std::string getActiveProfile() const override;
    std::vector<std::string> getProfiles() const override;

private:
    std::filesystem::path getSavePath(SaveSlot slot) const;
    std::filesystem::path getMetadataPath(SaveSlot slot) const;

    std::vector<ISaveable*> saveables_;
    std::string activeProfile_ = "default";
    std::chrono::seconds autoSaveInterval_{0};
    float autoSaveTimer_ = 0.0f;
    bool autoSaveEnabled_ = false;
    std::filesystem::path savesDirectory_ = "saves";
};

// Factory function (exported via namespace)
inline std::unique_ptr<ISaveSystem> createSaveSystem() {
    return std::make_unique<SaveSystem>();
}

}  // namespace jframe
