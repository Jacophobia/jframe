// bestow-script/src/ScriptLinter.cpp
// Pattern-based linter to detect hot-reload-breaking patterns in Lua code

module;

#include <spdlog/spdlog.h>

module bestow.script;

import std;

namespace bestow {

namespace {

// Check if a line starts a function or return block (simplified heuristic)
bool isInsideFunctionContext(const std::string& line) {
    // Look for patterns that indicate we're entering a function body
    // This is a simplified check - a full parser would be more accurate
    return line.find("function") != std::string::npos ||
           line.find("return {") != std::string::npos ||
           line.find("return{") != std::string::npos;
}

// Check if a line contains a top-level local capture of app.*
bool isTopLevelAppCapture(const std::string& line, std::string& capturedVar) {
    // Pattern: local <varname> = app.
    // We want to catch: local physics = app.systems.physics
    // But NOT: local physics = app.systems.physics inside a function

    // Find "local " at start (with optional leading whitespace)
    std::size_t localPos = line.find("local ");
    if (localPos == std::string::npos) {
        return false;
    }

    // Check if there's only whitespace before "local"
    for (std::size_t i = 0; i < localPos; ++i) {
        if (!std::isspace(static_cast<unsigned char>(line[i]))) {
            return false;  // Something before "local", probably inside a block
        }
    }

    // Find the variable name after "local "
    std::size_t varStart = localPos + 6;  // strlen("local ") = 6
    while (varStart < line.size() && std::isspace(static_cast<unsigned char>(line[varStart]))) {
        ++varStart;
    }

    std::size_t varEnd = varStart;
    while (varEnd < line.size() &&
           (std::isalnum(static_cast<unsigned char>(line[varEnd])) || line[varEnd] == '_')) {
        ++varEnd;
    }

    if (varStart >= varEnd) {
        return false;  // No variable name found
    }

    capturedVar = line.substr(varStart, varEnd - varStart);

    // Look for "= app." after the variable name
    std::size_t eqPos = line.find('=', varEnd);
    if (eqPos == std::string::npos) {
        return false;
    }

    // Skip whitespace after =
    std::size_t afterEq = eqPos + 1;
    while (afterEq < line.size() && std::isspace(static_cast<unsigned char>(line[afterEq]))) {
        ++afterEq;
    }

    // Check for "app."
    if (line.substr(afterEq, 4) == "app.") {
        return true;
    }

    return false;
}

// Check if a line is a comment
bool isComment(const std::string& line) {
    std::size_t firstNonSpace = 0;
    while (firstNonSpace < line.size() && std::isspace(static_cast<unsigned char>(line[firstNonSpace]))) {
        ++firstNonSpace;
    }
    return firstNonSpace < line.size() && line.substr(firstNonSpace, 2) == "--";
}

}  // namespace

LintResult lintLuaFile(const std::string& source, const std::string& filepath) {
    std::istringstream stream(source);
    std::string line;
    int lineNum = 0;
    bool inFunction = false;
    int braceDepth = 0;

    while (std::getline(stream, line)) {
        lineNum++;

        // Skip empty lines and comments
        if (line.empty() || isComment(line)) {
            continue;
        }

        // Track brace depth to determine if we're inside a function/table
        for (char c : line) {
            if (c == '{') braceDepth++;
            if (c == '}') braceDepth--;
        }

        // Track if we've entered a function context
        if (isInsideFunctionContext(line)) {
            inFunction = true;
        }

        // Only check for app captures at file scope (before any function/return)
        // and at brace depth 0 (not inside a table literal)
        if (!inFunction && braceDepth == 0) {
            std::string capturedVar;
            if (isTopLevelAppCapture(line, capturedVar)) {
                return LintResult{
                    .passed = false,
                    .message = std::format(
                        "{}:{}: 'local {} = app.*' at file scope breaks hot reload.\n"
                        "    Move this inside a function body instead:\n"
                        "\n"
                        "    -- WRONG (cached at load time):\n"
                        "    local {} = app.systems.something\n"
                        "\n"
                        "    -- RIGHT (resolved fresh each call):\n"
                        "    return {{\n"
                        "        update = function(self, dt)\n"
                        "            local {} = app.systems.something\n"
                        "            -- use it here\n"
                        "        end\n"
                        "    }}",
                        filepath, lineNum, capturedVar, capturedVar, capturedVar),
                    .lineNumber = lineNum
                };
            }
        }
    }

    return LintResult{.passed = true, .message = "", .lineNumber = 0};
}

}  // namespace bestow
