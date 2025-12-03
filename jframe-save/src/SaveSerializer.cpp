// jframe-save/src/SaveSerializer.cpp
// Archive implementations using cereal (PIMPL pattern for MSVC C++23 compatibility)

module;

#include <compare>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>

module jframe.save.impl;

namespace jframe {

//==========================================================================
// PIMPL implementations for archive classes
//==========================================================================

struct CerealSaveArchiveImpl {
    explicit CerealSaveArchiveImpl(std::ostream& stream)
        : archive(stream) {}

    cereal::BinaryOutputArchive archive;
};

struct CerealLoadArchiveImpl {
    explicit CerealLoadArchiveImpl(std::istream& stream)
        : archive(stream) {}

    cereal::BinaryInputArchive archive;
};

//==========================================================================
// CerealSaveArchive implementation
//==========================================================================

CerealSaveArchive::CerealSaveArchive(std::ostream& stream)
    : impl_(std::make_unique<CerealSaveArchiveImpl>(stream)) {}

CerealSaveArchive::~CerealSaveArchive() = default;

CerealSaveArchive::CerealSaveArchive(CerealSaveArchive&&) noexcept = default;
CerealSaveArchive& CerealSaveArchive::operator=(CerealSaveArchive&&) noexcept = default;

void CerealSaveArchive::writeInt(const std::string& key, int value) {
    impl_->archive(cereal::make_nvp(key.c_str(), value));
}

void CerealSaveArchive::writeFloat(const std::string& key, float value) {
    impl_->archive(cereal::make_nvp(key.c_str(), value));
}

void CerealSaveArchive::writeDouble(const std::string& key, double value) {
    impl_->archive(cereal::make_nvp(key.c_str(), value));
}

void CerealSaveArchive::writeString(const std::string& key, const std::string& value) {
    impl_->archive(cereal::make_nvp(key.c_str(), value));
}

void CerealSaveArchive::writeBool(const std::string& key, bool value) {
    impl_->archive(cereal::make_nvp(key.c_str(), value));
}

void CerealSaveArchive::writeBytes(const std::string& key, const std::vector<std::uint8_t>& value) {
    impl_->archive(cereal::make_nvp(key.c_str(), value));
}

void* CerealSaveArchive::getArchivePtr() {
    return &impl_->archive;
}

//==========================================================================
// CerealLoadArchive implementation
//==========================================================================

CerealLoadArchive::CerealLoadArchive(std::istream& stream)
    : impl_(std::make_unique<CerealLoadArchiveImpl>(stream)) {}

CerealLoadArchive::~CerealLoadArchive() = default;

CerealLoadArchive::CerealLoadArchive(CerealLoadArchive&&) noexcept = default;
CerealLoadArchive& CerealLoadArchive::operator=(CerealLoadArchive&&) noexcept = default;

int CerealLoadArchive::readInt(const std::string& key) const {
    int value = 0;
    impl_->archive(cereal::make_nvp(key.c_str(), value));
    return value;
}

float CerealLoadArchive::readFloat(const std::string& key) const {
    float value = 0.0f;
    impl_->archive(cereal::make_nvp(key.c_str(), value));
    return value;
}

double CerealLoadArchive::readDouble(const std::string& key) const {
    double value = 0.0;
    impl_->archive(cereal::make_nvp(key.c_str(), value));
    return value;
}

std::string CerealLoadArchive::readString(const std::string& key) const {
    std::string value;
    impl_->archive(cereal::make_nvp(key.c_str(), value));
    return value;
}

bool CerealLoadArchive::readBool(const std::string& key) const {
    bool value = false;
    impl_->archive(cereal::make_nvp(key.c_str(), value));
    return value;
}

std::vector<std::uint8_t> CerealLoadArchive::readBytes(const std::string& key) const {
    std::vector<std::uint8_t> value;
    impl_->archive(cereal::make_nvp(key.c_str(), value));
    return value;
}

void* CerealLoadArchive::getArchivePtr() const {
    return &impl_->archive;
}

}  // namespace jframe
