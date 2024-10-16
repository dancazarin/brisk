#include <catch2/catch_all.hpp>
#include "../core/Catch2Utils.hpp"
#include <brisk/network/Fetch.hpp>
#include <brisk/network/UserAgent.hpp>

using namespace Brisk;

TEST_CASE("Network tests") {
    using namespace std::literals;
    auto [response, bytes] = httpFetchBytes(HTTPRequest{
        .url     = "https://example.com",
        .method  = HTTPMethod::Head,
        .timeout = 10000ms,
    });
    fmt::println("error = {}", response.error);
    fmt::println("httpCode = {}", response.httpCode);
    fmt::println("effectiveUrl = {}", response.effectiveUrl);
    fmt::println("headers:\n{}", fmt::join(response.headers, "\n"));

    fmt::println("bytes.size() = {}", bytes.size());
    if (!bytes.empty()) {
        fmt::println("response: \n{}", toStringView(bytes));
    }
}
