// bestow-luabind/src/docs/DocFormatter.cpp
// Terminal formatting for API documentation with ANSI color support

module bestow.luabind;

import std;

namespace bestow {

DocFormatter::DocFormatter(bool useColor)
    : useColor_(useColor) {}

std::string DocFormatter::bold(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[1m" + text + "\033[0m";
}

std::string DocFormatter::dim(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[2m" + text + "\033[0m";
}

std::string DocFormatter::cyan(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[36m" + text + "\033[0m";
}

std::string DocFormatter::green(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[32m" + text + "\033[0m";
}

std::string DocFormatter::yellow(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[33m" + text + "\033[0m";
}

std::string DocFormatter::magenta(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[35m" + text + "\033[0m";
}

std::string DocFormatter::underline(const std::string& text) const {
    if (!useColor_) return text;
    return "\033[4m" + text + "\033[0m";
}

std::string DocFormatter::reset() const {
    if (!useColor_) return "";
    return "\033[0m";
}

std::string DocFormatter::formatSystemList(const DocRegistry& registry) const {
    std::string out;
    out += bold("Bestow API Reference") + "\n";
    out += dim(std::string(50, '=')) + "\n\n";

    const auto& systems = registry.getAllSystems();
    if (systems.empty()) {
        out += "No systems documented yet.\n";
        return out;
    }

    // Find max name length for alignment
    std::size_t maxLen = 0;
    for (const auto& sys : systems) {
        maxLen = std::max(maxLen, sys.qualifiedName.size());
    }

    for (const auto& sys : systems) {
        std::string padding(maxLen - sys.qualifiedName.size() + 2, ' ');
        out += "  " + cyan(sys.qualifiedName) + padding + dim(sys.description) + "\n";
    }

    out += "\n" + dim("Usage: bestow api <system>          Show system details") + "\n";
    out += dim("       bestow api <system>.<method> Show method details") + "\n";
    out += dim("       bestow api search <query>    Search all APIs") + "\n";

    return out;
}

std::string DocFormatter::formatSystem(const SystemDoc& system) const {
    std::string out;

    // Header
    out += bold(system.qualifiedName) + "\n";
    out += dim(std::string(system.qualifiedName.size(), '=')) + "\n";
    out += system.description + "\n\n";

    // Methods
    if (!system.methods.empty()) {
        out += bold("Methods:") + "\n";
        for (const auto& method : system.methods) {
            // Build parameter list
            std::string params;
            for (std::size_t i = 0; i < method.params.size(); ++i) {
                if (i > 0) params += ", ";
                if (method.params[i].optional) {
                    params += "[" + method.params[i].name + "]";
                } else {
                    params += method.params[i].name;
                }
            }

            // Build return type
            std::string ret;
            if (!method.returns.empty()) {
                ret = " -> " + green(method.returns[0].type);
            }

            std::string deprecated;
            if (method.deprecated) {
                deprecated = " " + yellow("[deprecated]");
            }

            out += "  " + cyan(method.name) + "(" + params + ")" + ret + deprecated + "\n";
        }
        out += "\n";
    }

    // Enums
    if (!system.enums.empty()) {
        out += bold("Enums:") + "\n";
        for (const auto& e : system.enums) {
            out += "  " + magenta(e.qualifiedName) + "\n";
            for (const auto& val : e.values) {
                out += "    " + val.name;
                if (!val.description.empty()) {
                    out += " - " + dim(val.description);
                }
                out += "\n";
            }
        }
        out += "\n";
    }

    // Types
    if (!system.types.empty()) {
        out += bold("Types:") + "\n";
        for (const auto& t : system.types) {
            out += "  " + magenta(t.name);
            if (!t.description.empty()) {
                out += " - " + dim(t.description);
            }
            out += "\n";
        }
        out += "\n";
    }

    // Properties
    if (!system.properties.empty()) {
        out += bold("Properties:") + "\n";
        for (const auto& prop : system.properties) {
            std::string ro = prop.readOnly ? " " + dim("[read-only]") : "";
            out += "  " + cyan(prop.name) + " : " + green(prop.type) + ro + "\n";
            if (!prop.description.empty()) {
                out += "    " + dim(prop.description) + "\n";
            }
        }
        out += "\n";
    }

    // See also
    if (!system.seeAlso.empty()) {
        out += dim("See also: ");
        for (std::size_t i = 0; i < system.seeAlso.size(); ++i) {
            if (i > 0) out += ", ";
            out += cyan(system.seeAlso[i]);
        }
        out += "\n";
    }

    return out;
}

std::string DocFormatter::formatMethod(const MethodDoc& method) const {
    std::string out;

    // Build signature
    std::string params;
    for (std::size_t i = 0; i < method.params.size(); ++i) {
        if (i > 0) params += ", ";
        params += method.params[i].name;
    }
    std::string ret;
    for (std::size_t i = 0; i < method.returns.size(); ++i) {
        if (i > 0) ret += ", ";
        ret += method.returns[i].type;
    }
    std::string sig = method.qualifiedName + "(" + params + ")";
    if (!ret.empty()) sig += " -> " + ret;

    out += bold(sig) + "\n";
    out += dim(std::string(sig.size(), '=')) + "\n";

    if (method.deprecated) {
        out += yellow("DEPRECATED") + "\n";
    }

    out += method.description + "\n";

    // Parameters
    if (!method.params.empty()) {
        out += "\n" + bold("Parameters:") + "\n";
        // Find max name + type length for alignment
        std::size_t maxLen = 0;
        for (const auto& p : method.params) {
            std::size_t len = p.name.size() + p.type.size() + 3;
            maxLen = std::max(maxLen, len);
        }
        for (const auto& p : method.params) {
            std::string nameType = p.name + " : " + green(p.type);
            // Raw length (without ANSI codes) for padding
            std::size_t rawLen = p.name.size() + 3 + p.type.size();
            std::string pad(maxLen > rawLen ? maxLen - rawLen + 2 : 2, ' ');

            std::string opt;
            if (p.optional) {
                opt = " " + dim("[optional");
                if (!p.defaultVal.empty()) {
                    opt += ", default: " + p.defaultVal;
                }
                opt += "]";
            }
            out += "  " + nameType + pad + p.description + opt + "\n";
        }
    }

    // Returns
    if (!method.returns.empty()) {
        out += "\n" + bold("Returns:") + "\n";
        for (const auto& r : method.returns) {
            out += "  " + green(r.type);
            if (!r.description.empty()) {
                out += "  " + r.description;
            }
            out += "\n";
        }
    }

    // Example
    if (!method.example.empty()) {
        out += "\n" + bold("Example:") + "\n";
        // Indent each line
        std::istringstream stream(method.example);
        std::string line;
        while (std::getline(stream, line)) {
            out += "  " + dim(line) + "\n";
        }
    }

    // See also
    if (!method.seeAlso.empty()) {
        out += "\n" + dim("See also: ");
        for (std::size_t i = 0; i < method.seeAlso.size(); ++i) {
            if (i > 0) out += ", ";
            out += cyan(method.seeAlso[i]);
        }
        out += "\n";
    }

    return out;
}

std::string DocFormatter::formatEnum(const EnumDoc& e) const {
    std::string out;

    out += bold(e.qualifiedName) + "\n";
    out += dim(std::string(e.qualifiedName.size(), '=')) + "\n";
    out += e.description + "\n\n";

    out += bold("Values:") + "\n";
    std::size_t maxLen = 0;
    for (const auto& val : e.values) {
        maxLen = std::max(maxLen, val.name.size());
    }
    for (const auto& val : e.values) {
        std::string pad(maxLen - val.name.size() + 2, ' ');
        out += "  " + cyan(val.name) + pad + dim(val.description) + "\n";
    }

    return out;
}

std::string DocFormatter::formatType(const TypeDoc& type) const {
    std::string out;

    out += bold(type.qualifiedName) + "\n";
    out += dim(std::string(type.qualifiedName.size(), '=')) + "\n";
    out += type.description + "\n\n";

    if (!type.fields.empty()) {
        out += bold("Fields:") + "\n";
        std::size_t maxLen = 0;
        for (const auto& f : type.fields) {
            maxLen = std::max(maxLen, f.name.size() + f.type.size() + 3);
        }
        for (const auto& f : type.fields) {
            std::string nameType = f.name + " : " + green(f.type);
            std::size_t rawLen = f.name.size() + 3 + f.type.size();
            std::string pad(maxLen > rawLen ? maxLen - rawLen + 2 : 2, ' ');
            std::string ro = f.readOnly ? " " + dim("[read-only]") : "";
            out += "  " + nameType + pad + f.description + ro + "\n";
        }
        out += "\n";
    }

    if (!type.methods.empty()) {
        out += bold("Methods:") + "\n";
        for (const auto& method : type.methods) {
            std::string params;
            for (std::size_t i = 0; i < method.params.size(); ++i) {
                if (i > 0) params += ", ";
                params += method.params[i].name;
            }
            std::string ret;
            if (!method.returns.empty()) {
                ret = " -> " + green(method.returns[0].type);
            }
            out += "  " + cyan(method.name) + "(" + params + ")" + ret + "\n";
            if (!method.description.empty()) {
                out += "    " + dim(method.description) + "\n";
            }
        }
        out += "\n";
    }

    if (!type.example.empty()) {
        out += bold("Example:") + "\n";
        std::istringstream stream(type.example);
        std::string line;
        while (std::getline(stream, line)) {
            out += "  " + dim(line) + "\n";
        }
    }

    return out;
}

std::string DocFormatter::formatSearchResults(
    const std::vector<DocRegistry::SearchResult>& results,
    const std::string& query) const {

    std::string out;
    if (results.empty()) {
        out += "No results found for '" + yellow(query) + "'\n";
        return out;
    }

    out += bold("Search results for '" + query + "'") + "\n";
    out += dim(std::string(30, '-')) + "\n\n";

    for (const auto& result : results) {
        std::string kind = dim("[" + result.kind + "]");
        out += "  " + cyan(result.qualifiedName) + " " + kind + "\n";
        if (!result.description.empty()) {
            // Truncate long descriptions
            std::string desc = result.description;
            if (desc.size() > 80) {
                desc = desc.substr(0, 77) + "...";
            }
            out += "    " + dim(desc) + "\n";
        }
    }

    out += "\n" + dim("Use 'bestow api <name>' for details") + "\n";
    return out;
}

std::string DocFormatter::formatNotFound(const std::string& query) const {
    std::string out;
    out += "Unknown API: '" + yellow(query) + "'\n\n";
    out += dim("Usage: bestow api                   List all systems") + "\n";
    out += dim("       bestow api <system>          Show system details") + "\n";
    out += dim("       bestow api <system>.<method> Show method details") + "\n";
    out += dim("       bestow api search <query>    Search all APIs") + "\n";
    return out;
}

} // namespace bestow
