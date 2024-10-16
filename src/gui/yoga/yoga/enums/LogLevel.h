/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// @generated by enums.py
// clang-format off
#pragma once

#include <cstdint>
#include <yoga/enums/YogaEnums.h>

namespace facebook::yoga {

enum class LogLevel : uint8_t {
  Error,
  Warn,
  Info,
  Debug,
  Verbose,
  Fatal,
};

template <>
constexpr int32_t ordinalCount<LogLevel>() {
  return 6;
}

const char* toString(LogLevel e);
} // namespace facebook::yoga
