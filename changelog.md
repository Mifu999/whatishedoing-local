# 1.5.4

- **Max Screenshots Kept** option: cap how many screenshots are kept in the `screenshots` folder. When a new screenshot pushes the folder over the limit, the oldest ones (by date) are deleted automatically. Set to 0 for no limit.

## 1.5.3

- The local log (`local_log.txt`) and its `screenshots` folder are now written to the mod's **save folder** (`.../GeometryDash/geode/mods/imes.whatishedoing-local`, under AppData on Windows) instead of the `config` folder in the GD install directory.

## 1.5.2

- Local screenshots: when local logging is on, each captured screenshot is saved as a PNG in a **`screenshots`** subfolder next to `local_log.txt`, and the log line now shows the **file name** (e.g. `screenshot: 2026-06-24_21-30-05_0.png`) instead of a generic marker.

## 1.5.1

- **Logging Destination** option: choose **Discord + Local file**, **Discord only**, or **Local file only**. In "Local file only" no webhook request is made at all.
- Repackaged as a **separate mod** (`imes.whatishedoing-local`) so it can be installed alongside the official mod and won't be overwritten by official updates. Has its own settings and its own `local_log.txt`.

## 1.5.0

- Optional **local log file**: mirror every notification to a plain text file (`local_log.txt` in the mod config folder), with the same messages, toggles and filters as the Discord webhook. Works even with no webhook URL set. Toggle it under **Local Log File** and open it with the **Open Log File** button.

## 1.4.2

- Seperate toggle for **Practice Run Complete!**

## 1.4.1

- Custom embed accent colors
- New logo

## 1.4.0

- New cheat detection system based on [Death Tracker](https://github.com/abb2k/death-tracker/)

## 1.3.1

- Removed **Ignore Noclip Runs** (it's broken on 1.3.0)

## 1.3.0

- Level upload/update notifications (by [MalikHw47](https://youtube.com/@MalikHw47))
- Per-slot custom text for upload/update notifications

## 1.2.2

- **New Best** minimum percentage

## 1.2.1

- Made the webhook display name optional

## 1.2.0

- Configurable screenshots
- Fixed some UI issues

## 1.1.3

- 4 extra webhook urls per profile

## 1.1.2

- Actually close the Geode settings page after loading a profile

## 1.1.1

- Profile manager uses button setting (with **Manage Profiles**) and other UI/UX + internal improvements
- Loading a profile closes Geode settings without the extra unsaved-changes discard prompt

## 1.1.0

- Profile manager

## 1.0.7

- Startpos level complete message
- Fixed **New Best** webhook during startpos bug

## 1.0.6

- Button to add/remove level ID on info page
- Local level ID support
- Don't send redacted messages
- Level ID filter applies to editor open/exit too
- Startpos progress

## 1.0.5

- Use Geode utils for splitting level ID list

## 1.0.4

- Level ID filter with All/Blacklist/Whitelist modes

## 1.0.3

- Send death message after a minimum percentage

## 1.0.2

- Reworked the codebase
- Optional blocking mode only for **Closed Geometry Dash**, all other webhooks use async
- Webhook stuff, HTTPS check, URL trim, optional `Retry-After` on 429, retry wait off game callback thread
- Removed buggy and useless idle tracker
- Removed "after {time}" text because it's already in the footer

## 1.0.1

- Option for using thread blocking requests
- Major code clean up

## 1.0.0

- Initial release
