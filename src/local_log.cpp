#include "local_log.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/file.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef GEODE_IS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

using namespace geode::prelude;

namespace local_log {

namespace {

// Serialize writes: logEmbed/logContent are normally called from the main
// thread, but the mutex makes the append safe regardless of caller context
// and prevents interleaved lines if that ever changes.
std::mutex& logMutex() {
    static std::mutex m;
    return m;
}

std::filesystem::path logFilePath() {
    return Mod::get()->getSaveDir() / "local_log.txt";
}

// Logging destination, parsed from the "log-destination" one-of setting.
// Values must stay byte-identical to the "one-of" list in mod.json.
enum class LogMode { Both, DiscordOnly, LocalOnly };

LogMode currentMode() {
    auto const s =
        Mod::get()->getSettingValue<std::string>("log-destination");
    if (s == "Local file only") {
        return LogMode::LocalOnly;
    }
    if (s == "Discord + Local file") {
        return LogMode::Both;
    }
    // Default and explicit "Discord only".
    return LogMode::DiscordOnly;
}

std::filesystem::path screenshotsDir() {
    return Mod::get()->getSaveDir() / "screenshots";
}

// Single local-time snapshot, reused so a log line and its screenshot filename
// share the exact same second.
std::tm nowLocalTm() {
    auto const now = std::chrono::system_clock::now();
    auto const tt = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &tt);
#else
    localtime_r(&tt, &local);
#endif
    return local;
}

std::string formatTm(std::tm const& t, char const* fmt) {
    char buf[32];
    std::strftime(buf, sizeof(buf), fmt, &t);
    return buf;
}

// Local wall-clock timestamp, e.g. "2026-06-24 14:32:05". Local time (not UTC)
// is intentional: this is a personal local trace and local time is the most
// intuitive for reviewing your own activity.
std::string currentLocalTimestamp() {
    return formatTm(nowLocalTm(), "%Y-%m-%d %H:%M:%S");
}

// Max number of screenshots to keep in the folder. 0 (or negative) = unlimited.
int screenshotLimit() {
    auto v = Mod::get()->getSettingValue<int64_t>("screenshot-limit");
    if (v < 0) {
        v = 0;
    }
    return static_cast<int>(v);
}

// Keep only the newest `limit` PNGs in the screenshots folder, deleting the
// oldest by last-write time (filename as a stable tiebreaker). No-op when the
// limit is 0/negative. Wrapped so a filesystem hiccup can never crash the game.
void pruneOldScreenshots(std::filesystem::path const& dir, int limit) {
    if (limit <= 0) {
        return;
    }
    try {
        std::vector<
            std::pair<std::filesystem::file_time_type, std::filesystem::path>
        > shots;
        std::error_code ec;
        for (auto const& entry :
             std::filesystem::directory_iterator(dir, ec)) {
            auto const& p = entry.path();
            if (p.extension() != ".png") {
                continue;
            }
            std::error_code fec;
            if (!entry.is_regular_file(fec)) {
                continue;
            }
            auto const t = std::filesystem::last_write_time(p, fec);
            if (fec) {
                continue;
            }
            shots.emplace_back(t, p);
        }

        if (static_cast<int>(shots.size()) <= limit) {
            return;
        }

        std::sort(
            shots.begin(),
            shots.end(),
            [](auto const& a, auto const& b) {
                if (a.first != b.first) {
                    return a.first < b.first; // oldest first
                }
                return a.second.filename().string()
                    < b.second.filename().string();
            }
        );

        std::size_t const toDelete =
            shots.size() - static_cast<std::size_t>(limit);
        for (std::size_t i = 0; i < toDelete; ++i) {
            std::error_code dec;
            std::filesystem::remove(shots[i].second, dec);
            if (dec) {
                log::warn(
                    "Local log: could not delete old screenshot {}",
                    shots[i].second.string()
                );
            }
        }
    } catch (std::exception const& e) {
        log::warn("Local log: screenshot pruning failed: {}", e.what());
    }
}

// Save already-encoded PNG bytes (the same bytes sent to Discord) into the
// "screenshots" subfolder. Returns the file name only (no path), or an empty
// string on failure. Filename base is the shared timestamp plus a monotonic
// sequence so two captures in the same second never collide.
std::string saveScreenshotToDisk(
    std::vector<std::uint8_t> const& png,
    std::string const& fileStamp
) {
    auto const dir = screenshotsDir();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    static std::atomic<std::uint64_t> seq{0};
    auto const n = seq.fetch_add(1, std::memory_order_relaxed);

    std::string const base = fmt::format("{}_{}", fileStamp, n);
    std::filesystem::path path = dir / (base + ".png");
    // Extremely unlikely, but guarantee we never clobber an existing file.
    int extra = 1;
    while (std::filesystem::exists(path, ec)) {
        path = dir / fmt::format("{}_{}.png", base, extra++);
    }

    auto res = geode::utils::file::writeBinary(path, png);
    if (!res.isOk()) {
        log::warn(
            "Local log: failed to write screenshot to {}",
            path.string()
        );
        return {};
    }

    // Enforce the rolling cap: delete the oldest screenshots if over the limit.
    pruneOldScreenshots(dir, screenshotLimit());

    return path.filename().string();
}

// Append a finished block (which already ends with a newline) plus one blank
// separator line. Opens in append mode so the file grows as a continuous trace
// and is flushed on close (important for the on-exit "Closed Geometry Dash"
// message, which is written while the process is shutting down).
void appendBlock(std::string const& block) {
    auto const path = logFilePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::lock_guard<std::mutex> guard(logMutex());
    std::ofstream out(
        path,
        std::ios::out | std::ios::app | std::ios::binary
    );
    if (!out.is_open()) {
        log::warn(
            "Local log: could not open file for appending: {}",
            path.string()
        );
        return;
    }
    out << block;
    out << "\n";
}

} // namespace

bool fileLoggingEnabled() {
    return currentMode() != LogMode::DiscordOnly;
}

bool discordEnabled() {
    return currentMode() != LogMode::LocalOnly;
}

void logEmbed(
    std::string const& title,
    std::string const& description,
    std::vector<WebhookField> const& fields,
    std::string const& footer,
    std::optional<std::vector<std::uint8_t>> const& screenshotPng
) {
    if (!fileLoggingEnabled()) {
        return;
    }

    // One timestamp for both the log line and any screenshot filename.
    std::tm const tm = nowLocalTm();
    std::string const stamp = formatTm(tm, "%Y-%m-%d %H:%M:%S");

    std::string block;
    block += fmt::format("[{}] {}\n", stamp, title);
    if (!description.empty()) {
        block += description;
        block += "\n";
    }
    for (auto const& f : fields) {
        block += fmt::format("  - {}: {}\n", f.name, f.value);
    }
    if (!footer.empty()) {
        block += fmt::format("  ({})\n", footer);
    }
    if (screenshotPng.has_value() && !screenshotPng->empty()) {
        std::string const fileStamp = formatTm(tm, "%Y-%m-%d_%H-%M-%S");
        std::string const name =
            saveScreenshotToDisk(*screenshotPng, fileStamp);
        if (!name.empty()) {
            block += fmt::format("  screenshot: {}\n", name);
        } else {
            block += "  screenshot: (failed to save)\n";
        }
    }
    appendBlock(block);
}

void logContent(std::string const& content) {
    if (!fileLoggingEnabled()) {
        return;
    }

    std::string block = fmt::format(
        "[{}] {}\n",
        currentLocalTimestamp(),
        content
    );
    appendBlock(block);
}

void openLogFile() {
    auto const path = logFilePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (!std::filesystem::exists(path, ec)) {
        // Create an empty file so there is something to open the first time.
        (void)geode::utils::file::writeString(path, "");
    }
#ifdef GEODE_IS_WINDOWS
    ShellExecuteW(
        nullptr,
        L"open",
        L"notepad.exe",
        path.wstring().c_str(),
        nullptr,
        SW_SHOW
    );
#else
    utils::file::openFolder(Mod::get()->getSaveDir());
#endif
}

} // namespace local_log
