/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include <yoga/Yoga.h>

#include <yoga/config/Config.h>
#include <yoga/enums/Dimension.h>
#include <yoga/enums/Direction.h>
#include <yoga/enums/Edge.h>
#include <yoga/enums/Errata.h>
#include <yoga/enums/MeasureMode.h>
#include <yoga/enums/NodeType.h>
#include <yoga/enums/PhysicalEdge.h>
#include <yoga/node/LayoutResults.h>
#include <yoga/style/Style.h>

namespace facebook::yoga {

using DirtiedFunc = void (*)(const Node* node);

struct Size {
  float width;
  float height;
};

/**
 * Returns the computed dimensions of the node, following the constraints of
 * `widthMode` and `heightMode`:
 *
 * YGMeasureModeUndefined: The parent has not imposed any constraint on the
 * child. It can be whatever size it wants.
 *
 * YGMeasureModeAtMost: The child can be as large as it wants up to the
 * specified size.
 *
 * YGMeasureModeExactly: The parent has determined an exact size for the
 * child. The child is going to be given those bounds regardless of how big it
 * wants to be.
 *
 * @returns the size of the leaf node, measured under the given constraints.
 */
using MeasureFunc = Size (*)(
    const Node* node,
    float width,
    MeasureMode widthMode,
    float height,
    MeasureMode heightMode);

/**
 * @returns a defined offset to baseline (ascent).
 */
using BaselineFunc = float (*)(const Node* node, float width, float height);

class Node;
struct NodeEnumerator;

class YG_EXPORT Node {
 public:
  virtual bool alwaysFormsContainingBlock() const = 0;
  virtual NodeType getNodeType() const = 0;
  virtual LayoutResults& getLayout() = 0;
  virtual const LayoutResults& getLayout() const = 0;
  virtual const Style& style() const = 0;
  virtual bool hasBaselineFunc() const noexcept = 0;
  virtual float baseline(float width, float height) const = 0;
  virtual Node* getChild(size_t index) const = 0;
  virtual size_t getChildCount() const = 0;
  virtual size_t getLineIndex() const = 0;
  virtual void setLineIndex(size_t lineIndex) = 0;
  virtual bool isReferenceBaseline() const = 0;
  virtual void setHasNewLayout() = 0;
  virtual bool isRoot() const = 0;
  virtual Style::Length getResolvedDimension(Dimension dimension) const = 0;
  virtual void resolveDimension() = 0;
  virtual void setDirty(bool isDirty) = 0;
  virtual bool isDirty() const = 0;
  virtual void markDirtyAndPropagate() = 0;
  virtual void zeroOutLayoutRecursively() = 0;

  virtual Size measure(
      float width,
      MeasureMode widthMode,
      float height,
      MeasureMode heightMode) = 0;
  virtual bool hasMeasureFunc() const noexcept = 0;

  void setPosition(
      Direction direction,
      float mainSize,
      float crossSize,
      float ownerWidth);
  NodeEnumerator getChildren() const;

  Direction resolveDirection(const Direction ownerDirection) {
    if (style().direction() == Direction::Inherit) {
      return ownerDirection != Direction::Inherit ? ownerDirection
                                                  : Direction::LTR;
    } else {
      return style().direction();
    }
  }
  float resolveFlexGrow() const;
  float resolveFlexShrink() const;
  float dimensionWithMargin(FlexDirection axis, float widthSize);
  Style::Length resolveFlexBasisPtr() const;
  bool isLayoutDimensionDefined(FlexDirection axis);

  /**
   * Whether the node has a "definite length" along the given axis.
   * https://www.w3.org/TR/css-sizing-3/#definite
   */
  inline bool hasDefiniteLength(Dimension dimension, float ownerSize) {
    auto usedValue = getResolvedDimension(dimension).resolve(ownerSize);
    return usedValue.isDefined() && usedValue.unwrap() >= 0.0f;
  }
  bool isNodeFlexible();

  void setLayoutPosition(float position, PhysicalEdge edge) {
    getLayout().setPosition(edge, position);
  }

  void setLayoutDirection(Direction direction) {
    getLayout().setDirection(direction);
  }

  void setLayoutMargin(float margin, PhysicalEdge edge) {
    getLayout().setMargin(edge, margin);
  }

  void setLayoutBorder(float border, PhysicalEdge edge) {
    getLayout().setBorder(edge, border);
  }

  void setLayoutPadding(float padding, PhysicalEdge edge) {
    getLayout().setPadding(edge, padding);
  }

  void setLayoutLastOwnerDirection(Direction direction) {
    getLayout().lastOwnerDirection = direction;
  }

  void setLayoutComputedFlexBasis(const FloatOptional computedFlexBasis) {
    getLayout().computedFlexBasis = computedFlexBasis;
  }

  void setLayoutComputedFlexBasisGeneration(
      uint32_t computedFlexBasisGeneration) {
    getLayout().computedFlexBasisGeneration = computedFlexBasisGeneration;
  }

  void setLayoutMeasuredDimension(
      float measuredDimension,
      Dimension dimension) {
    getLayout().setMeasuredDimension(dimension, measuredDimension);
  }

  void setLayoutHadOverflow(bool hadOverflow) {
    getLayout().setHadOverflow(hadOverflow);
  }

  void setLayoutDimension(float LengthValue, Dimension dimension) {
    getLayout().setDimension(dimension, LengthValue);
  }

  float relativePosition(
      FlexDirection axis,
      Direction direction,
      float axisSize) const;
};

struct NodeEnumerator {
  const Node* node;
  struct Iterator {
    const Node* node;
    size_t index;
    Iterator& operator++() noexcept {
      ++index;
      return *this;
    }
    Node* operator*() const noexcept {
      return node->getChild(index);
    }
    bool operator!=(std::nullptr_t) const noexcept {
      return index != node->getChildCount();
    }
  };
  Iterator begin() const noexcept {
    return {node, 0};
  }
  std::nullptr_t end() const noexcept {
    return {};
  }
};

inline NodeEnumerator Node::getChildren() const {
  return {this};
}

} // namespace facebook::yoga
