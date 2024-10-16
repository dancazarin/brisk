/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <atomic>
#include <yoga/Yoga.h>
#include <yoga/algorithm/FlexDirection.h>
#include <yoga/node/Node.h>

namespace facebook::yoga {

extern std::atomic<uint32_t> gCurrentGenerationCount;

bool calculateLayout(
    yoga::Node* node,
    float ownerWidth,
    float ownerHeight,
    Direction ownerDirection);

bool calculateLayoutInternal(
    yoga::Node* node,
    float availableWidth,
    float availableHeight,
    Direction ownerDirection,
    SizingMode widthSizingMode,
    SizingMode heightSizingMode,
    float ownerWidth,
    float ownerHeight,
    bool performLayout,
    uint32_t depth,
    uint32_t generationCount);

} // namespace facebook::yoga
