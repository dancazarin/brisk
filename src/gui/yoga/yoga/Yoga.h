/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

/**
 * `#include <yoga/Yoga.h>` includes all of Yoga's public headers.
 */

#include <type_traits>

#define YG_EXPORT
#define YG_DEPRECATED(message) [[deprecated(message)]]

namespace facebook::yoga {} // namespace facebook::yoga
