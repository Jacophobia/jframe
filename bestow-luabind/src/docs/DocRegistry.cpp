// bestow-luabind/src/docs/DocRegistry.cpp
// API documentation registry — stores and queries structured doc entries

module bestow.luabind;

import std;

namespace bestow {

void DocRegistry::addSystem(SystemDoc system) {
    systems_.push_back(std::move(system));
}

const std::vector<SystemDoc>& DocRegistry::getAllSystems() const {
    return systems_;
}

const SystemDoc* DocRegistry::getSystem(const std::string& name) const {
    for (const auto& sys : systems_) {
        if (sys.name == name) return &sys;
    }
    return nullptr;
}

const MethodDoc* DocRegistry::getMethod(const std::string& qualifiedName) const {
    // qualifiedName is e.g. "bestow.assets.registerAsset"
    // Parse system name from qualified name
    for (const auto& sys : systems_) {
        for (const auto& method : sys.methods) {
            if (method.qualifiedName == qualifiedName) return &method;
        }
        for (const auto& type : sys.types) {
            for (const auto& method : type.methods) {
                if (method.qualifiedName == qualifiedName) return &method;
            }
        }
    }
    return nullptr;
}

const EnumDoc* DocRegistry::getEnum(const std::string& qualifiedName) const {
    for (const auto& sys : systems_) {
        for (const auto& e : sys.enums) {
            if (e.qualifiedName == qualifiedName) return &e;
        }
    }
    return nullptr;
}

const TypeDoc* DocRegistry::getType(const std::string& qualifiedName) const {
    for (const auto& sys : systems_) {
        for (const auto& t : sys.types) {
            if (t.qualifiedName == qualifiedName) return &t;
        }
    }
    return nullptr;
}

namespace {

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) { return std::tolower(a) == std::tolower(b); }
    );
    return it != haystack.end();
}

} // anonymous namespace

std::vector<DocRegistry::SearchResult> DocRegistry::search(const std::string& query) const {
    std::vector<SearchResult> results;

    for (const auto& sys : systems_) {
        // Search system name/description
        if (containsIgnoreCase(sys.name, query) ||
            containsIgnoreCase(sys.description, query)) {
            results.push_back({sys.qualifiedName, "system", sys.description, sys.name});
        }

        // Search methods
        for (const auto& method : sys.methods) {
            if (containsIgnoreCase(method.name, query) ||
                containsIgnoreCase(method.description, query)) {
                results.push_back({method.qualifiedName, "method", method.description, sys.name});
            }
        }

        // Search enums
        for (const auto& e : sys.enums) {
            if (containsIgnoreCase(e.name, query) ||
                containsIgnoreCase(e.description, query)) {
                results.push_back({e.qualifiedName, "enum", e.description, sys.name});
            }
            for (const auto& val : e.values) {
                if (containsIgnoreCase(val.name, query) ||
                    containsIgnoreCase(val.description, query)) {
                    results.push_back({e.qualifiedName + "." + val.name, "enum value",
                                       val.description, sys.name});
                }
            }
        }

        // Search types
        for (const auto& t : sys.types) {
            if (containsIgnoreCase(t.name, query) ||
                containsIgnoreCase(t.description, query)) {
                results.push_back({t.qualifiedName, "type", t.description, sys.name});
            }
        }

        // Search properties
        for (const auto& prop : sys.properties) {
            if (containsIgnoreCase(prop.name, query) ||
                containsIgnoreCase(prop.description, query)) {
                results.push_back({sys.qualifiedName + "." + prop.name, "property",
                                   prop.description, sys.name});
            }
        }
    }

    return results;
}

} // namespace bestow
