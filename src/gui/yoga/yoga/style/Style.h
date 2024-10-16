/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include <yoga/Yoga.h>

#include <yoga/algorithm/FlexDirection.h>
#include <yoga/enums/Align.h>
#include <yoga/enums/Dimension.h>
#include <yoga/enums/Direction.h>
#include <yoga/enums/Display.h>
#include <yoga/enums/Edge.h>
#include <yoga/enums/FlexDirection.h>
#include <yoga/enums/Gutter.h>
#include <yoga/enums/Justify.h>
#include <yoga/enums/Overflow.h>
#include <yoga/enums/PhysicalEdge.h>
#include <yoga/enums/PositionType.h>
#include <yoga/enums/Unit.h>
#include <yoga/enums/Wrap.h>
#include <yoga/numeric/FloatOptional.h>
#include <yoga/style/StyleLength.h>

namespace facebook::yoga {

class YG_EXPORT Style {
 public:
  using Length = StyleLength;
  static constexpr float DefaultFlexGrow = 0.0f;
  static constexpr float DefaultFlexShrink = 0.0f;
  static constexpr float WebDefaultFlexShrink = 1.0f;

  virtual Direction direction() const = 0;
  virtual FlexDirection flexDirection() const = 0;
  virtual Justify justifyContent() const = 0;
  virtual Align alignContent() const = 0;
  virtual Align alignItems() const = 0;
  virtual Align alignSelf() const = 0;
  virtual PositionType positionType() const = 0;
  virtual Wrap flexWrap() const = 0;
  virtual Overflow overflow() const = 0;
  virtual Display display() const = 0;
  virtual FloatOptional flex() const = 0;
  virtual FloatOptional flexGrow() const = 0;
  virtual FloatOptional flexShrink() const = 0;
  virtual Style::Length flexBasis() const = 0;
  virtual Style::Length margin(Edge edge) const = 0;
  virtual Style::Length position(Edge edge) const = 0;
  virtual Style::Length padding(Edge edge) const = 0;
  virtual Style::Length border(Edge edge) const = 0;
  virtual Style::Length gap(Gutter gutter) const = 0;
  virtual Style::Length dimension(Dimension axis) const = 0;
  virtual Style::Length minDimension(Dimension axis) const = 0;
  virtual Style::Length maxDimension(Dimension axis) const = 0;
  virtual FloatOptional aspectRatio() const = 0;

  bool horizontalInsetsDefined() const {
    return position(Edge::Left).isDefined() ||
        position(Edge::Right).isDefined() || position(Edge::All).isDefined() ||
        position(Edge::Horizontal).isDefined() ||
        position(Edge::Start).isDefined() || position(Edge::End).isDefined();
  }

  bool verticalInsetsDefined() const {
    return position(Edge::Top).isDefined() ||
        position(Edge::Bottom).isDefined() || position(Edge::All).isDefined() ||
        position(Edge::Vertical).isDefined();
  }

  bool isFlexStartPositionDefined(FlexDirection axis, Direction direction)
      const {
    return computePosition(flexStartEdge(axis), direction).isDefined();
  }

  bool isInlineStartPositionDefined(FlexDirection axis, Direction direction)
      const {
    return computePosition(inlineStartEdge(axis, direction), direction)
        .isDefined();
  }

  bool isFlexEndPositionDefined(FlexDirection axis, Direction direction) const {
    return computePosition(flexEndEdge(axis), direction).isDefined();
  }

  bool isInlineEndPositionDefined(FlexDirection axis, Direction direction)
      const {
    return computePosition(inlineEndEdge(axis, direction), direction)
        .isDefined();
  }

  float computeFlexStartPosition(
      FlexDirection axis,
      Direction direction,
      float axisSize) const {
    return computePosition(flexStartEdge(axis), direction)
        .resolve(axisSize)
        .unwrapOrDefault(0.0f);
  }

  float computeInlineStartPosition(
      FlexDirection axis,
      Direction direction,
      float axisSize) const {
    return computePosition(inlineStartEdge(axis, direction), direction)
        .resolve(axisSize)
        .unwrapOrDefault(0.0f);
  }

  float computeFlexEndPosition(
      FlexDirection axis,
      Direction direction,
      float axisSize) const {
    return computePosition(flexEndEdge(axis), direction)
        .resolve(axisSize)
        .unwrapOrDefault(0.0f);
  }

  float computeInlineEndPosition(
      FlexDirection axis,
      Direction direction,
      float axisSize) const {
    return computePosition(inlineEndEdge(axis, direction), direction)
        .resolve(axisSize)
        .unwrapOrDefault(0.0f);
  }

  float computeFlexStartMargin(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeMargin(flexStartEdge(axis), direction)
        .resolve(widthSize)
        .unwrapOrDefault(0.0f);
  }

  float computeInlineStartMargin(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeMargin(inlineStartEdge(axis, direction), direction)
        .resolve(widthSize)
        .unwrapOrDefault(0.0f);
  }

  float computeFlexEndMargin(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeMargin(flexEndEdge(axis), direction)
        .resolve(widthSize)
        .unwrapOrDefault(0.0f);
  }

  float computeInlineEndMargin(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeMargin(inlineEndEdge(axis, direction), direction)
        .resolve(widthSize)
        .unwrapOrDefault(0.0f);
  }

  float computeFlexStartBorder(FlexDirection axis, Direction direction) const {
    return maxOrDefined(
        computeBorder(flexStartEdge(axis), direction).resolve(0.0f).unwrap(),
        0.0f);
  }

  float computeInlineStartBorder(FlexDirection axis, Direction direction)
      const {
    return maxOrDefined(
        computeBorder(inlineStartEdge(axis, direction), direction)
            .resolve(0.0f)
            .unwrap(),
        0.0f);
  }

  float computeFlexEndBorder(FlexDirection axis, Direction direction) const {
    return maxOrDefined(
        computeBorder(flexEndEdge(axis), direction).resolve(0.0f).unwrap(),
        0.0f);
  }

  float computeInlineEndBorder(FlexDirection axis, Direction direction) const {
    return maxOrDefined(
        computeBorder(inlineEndEdge(axis, direction), direction)
            .resolve(0.0f)
            .unwrap(),
        0.0f);
  }

  float computeFlexStartPadding(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return maxOrDefined(
        computePadding(flexStartEdge(axis), direction)
            .resolve(widthSize)
            .unwrap(),
        0.0f);
  }

  float computeInlineStartPadding(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return maxOrDefined(
        computePadding(inlineStartEdge(axis, direction), direction)
            .resolve(widthSize)
            .unwrap(),
        0.0f);
  }

  float computeFlexEndPadding(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return maxOrDefined(
        computePadding(flexEndEdge(axis), direction)
            .resolve(widthSize)
            .unwrap(),
        0.0f);
  }

  float computeInlineEndPadding(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return maxOrDefined(
        computePadding(inlineEndEdge(axis, direction), direction)
            .resolve(widthSize)
            .unwrap(),
        0.0f);
  }

  float computeInlineStartPaddingAndBorder(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeInlineStartPadding(axis, direction, widthSize) +
        computeInlineStartBorder(axis, direction);
  }

  float computeFlexStartPaddingAndBorder(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeFlexStartPadding(axis, direction, widthSize) +
        computeFlexStartBorder(axis, direction);
  }

  float computeInlineEndPaddingAndBorder(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeInlineEndPadding(axis, direction, widthSize) +
        computeInlineEndBorder(axis, direction);
  }

  float computeFlexEndPaddingAndBorder(
      FlexDirection axis,
      Direction direction,
      float widthSize) const {
    return computeFlexEndPadding(axis, direction, widthSize) +
        computeFlexEndBorder(axis, direction);
  }

  float computeBorderForAxis(FlexDirection axis) const {
    return computeInlineStartBorder(axis, Direction::LTR) +
        computeInlineEndBorder(axis, Direction::LTR);
  }

  float computeMarginForAxis(FlexDirection axis, float widthSize) const {
    // The total margin for a given axis does not depend on the direction
    // so hardcoding LTR here to avoid piping direction to this function
    return computeInlineStartMargin(axis, Direction::LTR, widthSize) +
        computeInlineEndMargin(axis, Direction::LTR, widthSize);
  }

  float computeGapForAxis(FlexDirection axis, float ownerSize) const {
    auto gap = isRow(axis) ? computeColumnGap() : computeRowGap();
    return maxOrDefined(gap.resolve(ownerSize).unwrap(), 0.0f);
  }

  bool flexStartMarginIsAuto(FlexDirection axis, Direction direction) const {
    return computeMargin(flexStartEdge(axis), direction).isAuto();
  }

  bool flexEndMarginIsAuto(FlexDirection axis, Direction direction) const {
    return computeMargin(flexEndEdge(axis), direction).isAuto();
  }

 private:
  using EdgeFunction = Style::Length (Style::*)(Edge edge) const;
  Style::Length computePosition(PhysicalEdge edge, Direction direction) const {
    switch (edge) {
      case PhysicalEdge::Left:
        return computeLeftEdge(&Style::position, direction);
      case PhysicalEdge::Top:
        return computeTopEdge(&Style::position);
      case PhysicalEdge::Right:
        return computeRightEdge(&Style::position, direction);
      case PhysicalEdge::Bottom:
        return computeBottomEdge(&Style::position);
    }

    fatalWithMessage("Invalid physical edge");
  }

  Style::Length computeMargin(PhysicalEdge edge, Direction direction) const {
    switch (edge) {
      case PhysicalEdge::Left:
        return computeLeftEdge(&Style::margin, direction);
      case PhysicalEdge::Top:
        return computeTopEdge(&Style::margin);
      case PhysicalEdge::Right:
        return computeRightEdge(&Style::margin, direction);
      case PhysicalEdge::Bottom:
        return computeBottomEdge(&Style::margin);
    }

    fatalWithMessage("Invalid physical edge");
  }

  Style::Length computePadding(PhysicalEdge edge, Direction direction) const {
    switch (edge) {
      case PhysicalEdge::Left:
        return computeLeftEdge(&Style::padding, direction);
      case PhysicalEdge::Top:
        return computeTopEdge(&Style::padding);
      case PhysicalEdge::Right:
        return computeRightEdge(&Style::padding, direction);
      case PhysicalEdge::Bottom:
        return computeBottomEdge(&Style::padding);
    }

    fatalWithMessage("Invalid physical edge");
  }

  Style::Length computeBorder(PhysicalEdge edge, Direction direction) const {
    switch (edge) {
      case PhysicalEdge::Left:
        return computeLeftEdge(&Style::border, direction);
      case PhysicalEdge::Top:
        return computeTopEdge(&Style::border);
      case PhysicalEdge::Right:
        return computeRightEdge(&Style::border, direction);
      case PhysicalEdge::Bottom:
        return computeBottomEdge(&Style::border);
    }

    fatalWithMessage("Invalid physical edge");
  }

  Style::Length computeColumnGap() const {
    if (gap(Gutter::Column).isDefined()) {
      return gap(Gutter::Column);
    } else {
      return gap(Gutter::All);
    }
  }

  Style::Length computeRowGap() const {
    if (gap(Gutter::Row).isDefined()) {
      return gap(Gutter::Row);
    } else {
      return gap(Gutter::All);
    }
  }
  Style::Length computeLeftEdge(EdgeFunction edges, Direction layoutDirection)
      const {
    if (layoutDirection == Direction::LTR &&
        (this->*edges)(Edge::Start).isDefined()) {
      return (this->*edges)(Edge::Start);
    } else if (
        layoutDirection == Direction::RTL &&
        (this->*edges)(Edge::End).isDefined()) {
      return (this->*edges)(Edge::End);
    } else if ((this->*edges)(Edge::Left).isDefined()) {
      return (this->*edges)(Edge::Left);
    } else if ((this->*edges)(Edge::Horizontal).isDefined()) {
      return (this->*edges)(Edge::Horizontal);
    } else {
      return (this->*edges)(Edge::All);
    }
  }

  Style::Length computeTopEdge(EdgeFunction edges) const {
    if ((this->*edges)(Edge::Top).isDefined()) {
      return (this->*edges)(Edge::Top);
    } else if ((this->*edges)(Edge::Vertical).isDefined()) {
      return (this->*edges)(Edge::Vertical);
    } else {
      return (this->*edges)(Edge::All);
    }
  }

  Style::Length computeRightEdge(EdgeFunction edges, Direction layoutDirection)
      const {
    if (layoutDirection == Direction::LTR &&
        (this->*edges)(Edge::End).isDefined()) {
      return (this->*edges)(Edge::End);
    } else if (
        layoutDirection == Direction::RTL &&
        (this->*edges)(Edge::Start).isDefined()) {
      return (this->*edges)(Edge::Start);
    } else if ((this->*edges)(Edge::Right).isDefined()) {
      return (this->*edges)(Edge::Right);
    } else if ((this->*edges)(Edge::Horizontal).isDefined()) {
      return (this->*edges)(Edge::Horizontal);
    } else {
      return (this->*edges)(Edge::All);
    }
  }

  Style::Length computeBottomEdge(EdgeFunction edges) const {
    if ((this->*edges)(Edge::Bottom).isDefined()) {
      return (this->*edges)(Edge::Bottom);
    } else if ((this->*edges)(Edge::Vertical).isDefined()) {
      return (this->*edges)(Edge::Vertical);
    } else {
      return (this->*edges)(Edge::All);
    }
  }
};

} // namespace facebook::yoga
