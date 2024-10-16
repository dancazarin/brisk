/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <yoga/debug/Log.h>
#include <cstdarg>

#ifdef ANDROID
#include <android/log.h>
#endif

namespace facebook::yoga {

namespace {

void vlog(
    const yoga::Node* /* node */,
    LogLevel /* level */,
    const char* /* format */,
    va_list /* args */) {}
} // namespace

void log(LogLevel level, const char* format, ...) noexcept {
  va_list args;
  va_start(args, format);
  vlog(nullptr, level, format, args);
  va_end(args);
}

void log(
    const yoga::Node* node,
    LogLevel level,
    const char* format,
    ...) noexcept {
  va_list args;
  va_start(args, format);
  vlog(node, level, format, args);
  va_end(args);
}

} // namespace facebook::yoga
