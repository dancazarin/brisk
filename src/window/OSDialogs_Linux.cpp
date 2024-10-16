/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <brisk/window/OSDialogs.hpp>
#include <brisk/window/WindowApplication.hpp>

namespace Brisk {

DialogResult showDialog(std::string_view title, std::string_view message, DialogButtons buttons,
                        MessageBoxType type) {
    LOG_WARN(OSDialogs, "Not implemented");
    return {};
}

void openURLInBrowser(std::string_view url) {
    LOG_WARN(OSDialogs, "Not implemented");
}

void openFileInDefaultApp(const fs::path& path) {
    LOG_WARN(OSDialogs, "Not implemented");
}

void openFolder(const fs::path& path) {
    LOG_WARN(OSDialogs, "Not implemented");
}

optional<fs::path> showOpenDialog(std::span<const FileDialogFilter> filters, const fs::path& defaultPath) {
    LOG_WARN(OSDialogs, "Not implemented");
    return {};
}

std::vector<fs::path> showOpenDialogMulti(std::span<const FileDialogFilter> filters,
                                          const fs::path& defaultPath) {
    LOG_WARN(OSDialogs, "Not implemented");
    return {};
}

optional<fs::path> showSaveDialog(std::span<const FileDialogFilter> filters, const fs::path& defaultPath) {
    LOG_WARN(OSDialogs, "Not implemented");
    return {};
}

optional<fs::path> showFolderDialog(const fs::path& defaultPath) {
    LOG_WARN(OSDialogs, "Not implemented");
    return {};
}
} // namespace Brisk
