#pragma once

#include "webhook.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Local text-file mirror of every notification the mod sends to Discord.
// These functions are called from the same central send points as the
// webhook (sendImpl / sendContentImpl), so the local trace honors all the
// same notification toggles, filters and redaction. Each logging function is a
// no-op unless the "log-destination" setting includes the local file. Local
// logging runs even when no webhook URL is configured.
namespace local_log {

// Logging destination, driven by the "log-destination" setting:
//   "Discord + Local file" -> both true
//   "Discord only"         -> discord true,  file false
//   "Local file only"      -> discord false, file true
// fileLoggingEnabled() gates the local writes below; discordEnabled() is used
// by the webhook sender to decide whether to perform the network request.
bool fileLoggingEnabled();
bool discordEnabled();

// Append an embed-style notification (the same title/description/fields/footer
// that get built into the Discord embed). If screenshotPng holds bytes and
// local file logging is enabled, the PNG is also saved into a "screenshots"
// subfolder next to local_log.txt, and the log line shows the saved file name
// (e.g. "screenshot: 2026-06-24_21-30-05_0.png") instead of a generic marker.
void logEmbed(
    std::string const& title,
    std::string const& description,
    std::vector<WebhookField> const& fields,
    std::string const& footer,
    std::optional<std::vector<std::uint8_t>> const& screenshotPng
);

// Append a plain-content notification (the "content" field webhooks, e.g.
// custom-text level uploads).
void logContent(std::string const& content);

// Open the local log file for the user (Windows: notepad; otherwise opens the
// mod config folder). Creates an empty file first if it does not exist yet.
void openLogFile();

} // namespace local_log
