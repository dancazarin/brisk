/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <algorithm>
#include <cstddef>
#include <iostream>

#include <yoga/debug/AssertFatal.h>
#include <yoga/node/Node.h>
#include <yoga/numeric/Comparison.h>

namespace facebook::yoga {

// If both left and right are defined, then use left. Otherwise return +left or
// -right depending on which is defined. Ignore statically positioned nodes as
// insets do not apply to them.
float Node::relativePosition(
    FlexDirection axis,
    Direction direction,
    float axisSize) const {
  auto& style_ = style();
  if (style_.positionType() == PositionType::Static) {
    return 0;
  }
  if (style_.isInlineStartPositionDefined(axis, direction)) {
    return style_.computeInlineStartPosition(axis, direction, axisSize);
  }

  return -1 * style_.computeInlineEndPosition(axis, direction, axisSize);
}

Style::Length Node::resolveFlexBasisPtr() const {
  auto& style_ = style();
  Style::Length flexBasis = style_.flexBasis();
  if (flexBasis.unit() != Unit::Auto && flexBasis.unit() != Unit::Undefined) {
    return flexBasis;
  }
  if (style_.flex().isDefined() && style_.flex().unwrap() > 0.0f) {
    return useWebDefaults ? value::ofAuto() : value::points(0);
  }
  return value::ofAuto();
}
float Node::resolveFlexShrink() const {
  auto& style_ = style();
  if (isRoot()) {
    return 0.0;
  }
  if (style_.flexShrink().isDefined()) {
    return style_.flexShrink().unwrap();
  }
  if (!useWebDefaults && style_.flex().isDefined() &&
      style_.flex().unwrap() < 0.0f) {
    return -style_.flex().unwrap();
  }
  return useWebDefaults ? Style::WebDefaultFlexShrink
                        : Style::DefaultFlexShrink;
}
float Node::resolveFlexGrow() const {
  auto& style_ = style();
  // Root nodes flexGrow should always be 0
  if (isRoot()) {
    return 0.0;
  }
  if (style_.flexGrow().isDefined()) {
    return style_.flexGrow().unwrap();
  }
  if (style_.flex().isDefined() && style_.flex().unwrap() > 0.0f) {
    return style_.flex().unwrap();
  }
  return Style::DefaultFlexGrow;
}
float Node::dimensionWithMargin(
    const FlexDirection axis,
    const float widthSize) {
  return getLayout().measuredDimension(dimension(axis)) +
      style().computeMarginForAxis(axis, widthSize);
}
bool Node::isLayoutDimensionDefined(const FlexDirection axis) {
  const float value = getLayout().measuredDimension(dimension(axis));
  return yoga::isDefined(value) && value >= 0.0f;
}
bool Node::isNodeFlexible() {
  return (
      (style().positionType() != PositionType::Absolute) &&
      (resolveFlexGrow() != 0 || resolveFlexShrink() != 0));
}
void Node::setPosition(
    const Direction direction,
    const float mainSize,
    const float crossSize,
    const float ownerWidth) {
  auto& style_ = style();
  /* Root nodes should be always layouted as LTR, so we don't return negative
   * values. */
  const Direction directionRespectingRoot =
      !isRoot() ? direction : Direction::LTR;
  const FlexDirection mainAxis =
      yoga::resolveDirection(style_.flexDirection(), directionRespectingRoot);
  const FlexDirection crossAxis =
      yoga::resolveCrossDirection(mainAxis, directionRespectingRoot);

  // In the case of position static these are just 0. See:
  // https://www.w3.org/TR/css-position-3/#valdef-position-static
  const float relativePositionMain =
      relativePosition(mainAxis, directionRespectingRoot, mainSize);
  const float relativePositionCross =
      relativePosition(crossAxis, directionRespectingRoot, crossSize);

  const auto mainAxisLeadingEdge = inlineStartEdge(mainAxis, direction);
  const auto mainAxisTrailingEdge = inlineEndEdge(mainAxis, direction);
  const auto crossAxisLeadingEdge = inlineStartEdge(crossAxis, direction);
  const auto crossAxisTrailingEdge = inlineEndEdge(crossAxis, direction);

  setLayoutPosition(
      (style_.computeInlineStartMargin(mainAxis, direction, ownerWidth) +
       relativePositionMain),
      mainAxisLeadingEdge);
  setLayoutPosition(
      (style_.computeInlineEndMargin(mainAxis, direction, ownerWidth) +
       relativePositionMain),
      mainAxisTrailingEdge);
  setLayoutPosition(
      (style_.computeInlineStartMargin(crossAxis, direction, ownerWidth) +
       relativePositionCross),
      crossAxisLeadingEdge);
  setLayoutPosition(
      (style_.computeInlineEndMargin(crossAxis, direction, ownerWidth) +
       relativePositionCross),
      crossAxisTrailingEdge);
}
} // namespace facebook::yoga
