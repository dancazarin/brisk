#include <brisk/network/UserAgent.hpp>
#include <brisk/core/Brisk.h>

#include <brisk/core/App.hpp>

namespace Brisk {

std::optional<std::string> httpUserAgent;

std::string defaultHttpUserAgent() {
    return std::string("Mozilla/5.0 (") + std::string(appMetadata.vendor) + " " +
           std::string(appMetadata.name) + " " + appMetadata.version.string() + "; " BRISK_OS_NAME ")";
}
} // namespace Brisk
