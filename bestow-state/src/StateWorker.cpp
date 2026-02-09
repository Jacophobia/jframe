// bestow-state/src/StateWorker.cpp
// Background thread for async commit/restore operations

module;

#include <spdlog/spdlog.h>

module bestow.state.impl;

namespace bestow {

StateWorker::StateWorker() = default;

StateWorker::~StateWorker() {
    stop();
}

void StateWorker::start(const std::filesystem::path& dbPath) {
    if (running_.load()) return;

    dbPath_ = dbPath;
    running_.store(true);
    thread_ = std::thread(&StateWorker::workerLoop, this);
    spdlog::debug("[State] Worker thread started");
}

void StateWorker::stop() {
    if (!running_.load()) return;

    {
        std::lock_guard lock(queueMutex_);
        Command cmd;
        cmd.type = Command::Type::Stop;
        commandQueue_.push_back(std::move(cmd));
    }
    queueCV_.notify_one();

    if (thread_.joinable()) {
        thread_.join();
    }
    running_.store(false);
    spdlog::debug("[State] Worker thread stopped");
}

void StateWorker::enqueueCommit(CommitCommand cmd) {
    {
        std::lock_guard lock(queueMutex_);
        Command c;
        c.type = Command::Type::Commit;
        c.commitCmd = std::move(cmd);
        commandQueue_.push_back(std::move(c));
    }
    queueCV_.notify_one();
}

void StateWorker::enqueueRestore(RestoreCommand cmd) {
    {
        std::lock_guard lock(queueMutex_);
        Command c;
        c.type = Command::Type::Restore;
        c.restoreCmd = std::move(cmd);
        commandQueue_.push_back(std::move(c));
    }
    queueCV_.notify_one();
}

std::vector<StateWorker::CompletedOp> StateWorker::pollCompleted() {
    std::lock_guard lock(completedMutex_);
    std::vector<CompletedOp> result;
    result.swap(completedOps_);
    return result;
}

void StateWorker::workerLoop() {
    // Worker thread owns its own database connection
    StateDatabase db;
    if (!db.open(dbPath_)) {
        spdlog::error("[State] Worker failed to open database");
        return;
    }

    while (true) {
        std::vector<Command> batch;

        {
            std::unique_lock lock(queueMutex_);
            queueCV_.wait(lock, [this] { return !commandQueue_.empty(); });
            batch.swap(commandQueue_);
        }

        for (auto& cmd : batch) {
            switch (cmd.type) {
                case Command::Type::Commit: {
                    bool ok = db.writeSlot(cmd.commitCmd.slot, cmd.commitCmd.data,
                                           cmd.commitCmd.metadata);
                    CompletedOp op;
                    op.type = CompletedOp::Type::Commit;
                    op.success = ok;
                    op.error = ok ? StateError::Success : StateError::DatabaseError;
                    op.callback = std::move(cmd.commitCmd.callback);
                    op.slot = cmd.commitCmd.slot;

                    std::lock_guard lock(completedMutex_);
                    completedOps_.push_back(std::move(op));
                    break;
                }
                case Command::Type::Restore: {
                    auto data = db.readSlot(cmd.restoreCmd.slot);
                    bool ok = db.slotExists(cmd.restoreCmd.slot);

                    CompletedOp op;
                    op.type = CompletedOp::Type::Restore;
                    op.success = ok;
                    op.error = ok ? StateError::Success : StateError::SlotNotFound;
                    op.callback = std::move(cmd.restoreCmd.callback);
                    op.restoredData = std::move(data);
                    op.slot = cmd.restoreCmd.slot;

                    std::lock_guard lock(completedMutex_);
                    completedOps_.push_back(std::move(op));
                    break;
                }
                case Command::Type::Delete: {
                    db.deleteSlot(cmd.deleteCmd.slot);
                    break;
                }
                case Command::Type::Stop: {
                    db.close();
                    return;
                }
            }
        }
    }
}

}  // namespace bestow
