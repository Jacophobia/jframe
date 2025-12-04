// bestow-save/src/bestow.save.impl.cppm
// Save system implementation
//
// Note: cereal and nlohmann/json headers are NOT included here due to MSVC C++23
// module compatibility issues. All cereal/JSON usage is confined to SaveSystem.cpp
// using the PIMPL pattern for archive classes.

module;

// No third-party headers in global module fragment for MSVC compatibility

export module bestow.save.impl;

import std;
import bestow.save;
import bestow.types;

export namespace bestow {

// Forward declaration for PIMPL
struct CerealSaveArchiveImpl;
struct CerealLoadArchiveImpl;

// Concrete SaveArchive implementing ISaveArchive
// Uses PIMPL to hide cereal types from module interface (MSVC C++23 compatibility)
class CerealSaveArchive : public ISaveArchive {
public:
    explicit CerealSaveArchive(std::ostream& stream);
    ~CerealSaveArchive() override;

    // Non-copyable, movable
    CerealSaveArchive(const CerealSaveArchive&) = delete;
    CerealSaveArchive& operator=(const CerealSaveArchive&) = delete;
    CerealSaveArchive(CerealSaveArchive&&) noexcept;
    CerealSaveArchive& operator=(CerealSaveArchive&&) noexcept;

    void writeInt(const std::string& key, int value) override;
    void writeFloat(const std::string& key, float value) override;
    void writeDouble(const std::string& key, double value) override;
    void writeString(const std::string& key, const std::string& value) override;
    void writeBool(const std::string& key, bool value) override;
    void writeBytes(const std::string& key, const std::vector<std::uint8_t>& value) override;

    // Direct access for internal use (returns opaque pointer, cast in .cpp)
    void* getArchivePtr();

private:
    std::unique_ptr<CerealSaveArchiveImpl> impl_;
};

// Concrete LoadArchive implementing ILoadArchive
// Uses PIMPL to hide cereal types from module interface (MSVC C++23 compatibility)
class CerealLoadArchive : public ILoadArchive {
public:
    explicit CerealLoadArchive(std::istream& stream);
    ~CerealLoadArchive() override;

    // Non-copyable, movable
    CerealLoadArchive(const CerealLoadArchive&) = delete;
    CerealLoadArchive& operator=(const CerealLoadArchive&) = delete;
    CerealLoadArchive(CerealLoadArchive&&) noexcept;
    CerealLoadArchive& operator=(CerealLoadArchive&&) noexcept;

    int readInt(const std::string& key) const override;
    float readFloat(const std::string& key) const override;
    double readDouble(const std::string& key) const override;
    std::string readString(const std::string& key) const override;
    bool readBool(const std::string& key) const override;
    std::vector<std::uint8_t> readBytes(const std::string& key) const override;

    // Direct access for internal use (returns opaque pointer, cast in .cpp)
    void* getArchivePtr() const;

private:
    std::unique_ptr<CerealLoadArchiveImpl> impl_;
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

}  // namespace bestow
