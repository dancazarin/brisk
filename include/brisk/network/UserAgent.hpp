#pragma once

#include <string>
#include <optional>

namespace Brisk {

std::string defaultHttpUserAgent();

/// If empty, value returned by defaultHttpUserAgent() is used
extern std::optional<std::string> httpUserAgent;

} // namespace Brisk
