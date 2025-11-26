// jframe-dev/src/HotReloadManager.cpp
// Hot reload implementation using efsw

module;

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <efsw/efsw.hpp>
#include <sol/sol.hpp>

module jframe.dev;

namespace jframe::dev {

struct HotReloadManager::Impl : public efsw::FileWatchListener {
    efsw::FileWatcher watcher;
    std::queue<FileChange> pendingChanges;
    std::mutex changeMutex;
    sol::state lua;

    void handleFileAction(efsw::WatchID watchid,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action action,
                          std::string oldFilename) override {
        FileChange::Action changeAction;
        switch (action) {
            case efsw::Actions::Add:
                changeAction = FileChange::Action::Added;
                break;
            case efsw::Actions::Modified:
                changeAction = FileChange::Action::Modified;
                break;
            case efsw::Actions::Delete:
                changeAction = FileChange::Action::Deleted;
                break;
            default:
                return;
        }

        std::lock_guard lock(changeMutex);
        pendingChanges.push({std::filesystem::path(dir) / filename, changeAction});
    }
};

HotReloadManager::HotReloadManager() : impl_(std::make_unique<Impl>()) {
    // Set up sandboxed Lua state
    impl_->lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);

    // Remove dangerous functions
    impl_->lua["os"] = sol::lua_nil;
    impl_->lua["io"] = sol::lua_nil;
    impl_->lua["loadfile"] = sol::lua_nil;
    impl_->lua["dofile"] = sol::lua_nil;
    impl_->lua["load"] = sol::lua_nil;
}

HotReloadManager::~HotReloadManager() {
    stopWatching();
}

void HotReloadManager::watchDirectory(const std::filesystem::path& dir) {
    impl_->watcher.addWatch(dir.string(), impl_.get(), true);
    impl_->watcher.watch();
}

void HotReloadManager::stopWatching() {
    // efsw handles cleanup
}

void HotReloadManager::update() {
    std::queue<FileChange> toProcess;
    {
        std::lock_guard lock(impl_->changeMutex);
        std::swap(toProcess, impl_->pendingChanges);
    }

    while (!toProcess.empty()) {
        const auto& change = toProcess.front();

        if (change.action != FileChange::Action::Deleted) {
            std::string ext = change.path.extension().string();
            std::string pathStr = change.path.string();

            if (pathStr.find("blueprints") != std::string::npos && ext == ".lua") {
                if (onBlueprintChanged) onBlueprintChanged(change.path);
            } else if (pathStr.find("levels") != std::string::npos && ext == ".lua") {
                if (onLevelChanged) onLevelChanged(change.path);
            } else if (ext == ".png" || ext == ".jpg") {
                if (onTextureChanged) onTextureChanged(change.path);
            } else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
                if (onAudioChanged) onAudioChanged(change.path);
            } else if (pathStr.find("config") != std::string::npos && ext == ".lua") {
                if (onConfigChanged) onConfigChanged(change.path);
            }
        }

        toProcess.pop();
    }
}

}  // namespace jframe::dev
