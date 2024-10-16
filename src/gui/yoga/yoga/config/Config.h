/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <bitset>

#include <yoga/Yoga.h>
#include <yoga/enums/Errata.h>
#include <yoga/enums/ExperimentalFeature.h>
#include <yoga/enums/LogLevel.h>

namespace facebook::yoga {

using ExperimentalFeatureSet = std::bitset<ordinalCount<ExperimentalFeature>()>;

constexpr inline bool useWebDefaults = false;

constexpr inline ExperimentalFeatureSet enabledExperiments{1u};

constexpr inline bool isExperimentalFeatureEnabled(
    ExperimentalFeature experimentalFeature) {
  return enabledExperiments[to_underlying(experimentalFeature)];
}

constexpr inline Errata errata = Errata::None;

constexpr inline bool hasErrata(Errata err) {
  return (err & errata) != Errata::None;
}

inline float pointScaleFactor = 1.f;

} // namespace facebook::yoga
