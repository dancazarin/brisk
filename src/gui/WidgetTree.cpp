/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <brisk/gui/WidgetTree.hpp>
#include <brisk/gui/GUI.hpp>

namespace Brisk {

WidgetGroup::~WidgetGroup() {
    for (Widget* w : widgets) {
        w->removeFromGroup(this);
    }
}

void WidgetTree::setRoot(std::shared_ptr<Widget> root) {
    if (root != m_root) {
        if (m_root) {
            m_root->setTree(nullptr);
        }
        m_root = std::move(root);
        if (m_root) {
            m_root->setTree(this);
        }
    }
}

void WidgetTree::rescale() {
    if (m_root) {
        LOG_INFO(tree, "rescale");
        m_root->resolveProperties(PropFlags::AffectLayout | PropFlags::AffectFont);
    }
}

void WidgetTree::onLayoutUpdated() {
    ++m_layoutCounter;
}

void WidgetTree::processAnimation() {
    auto queue       = std::move(m_animationQueue);
    m_animationQueue = {};
    for (const auto& weak : queue) {
        if (auto strong = weak.lock()) {
            strong->animationFrame();
        }
    }
}

void WidgetTree::processRebuild() {
    auto queue     = std::move(m_rebuildQueue);
    m_rebuildQueue = {};
    for (const auto& weak : queue) {
        if (auto strong = weak.lock()) {
            strong->doRebuild();
        }
    }
}

void WidgetTree::requestAnimationFrame(std::weak_ptr<Widget> widget) {
    m_animationQueue.push_back(std::move(widget));
}

void WidgetTree::requestRebuild(std::weak_ptr<Widget> widget) {
    m_rebuildQueue.push_back(std::move(widget));
}

void WidgetTree::requestLayer(Drawable drawable) {
    m_layer.push_back(std::move(drawable));
}

uint32_t WidgetTree::layoutCounter() const noexcept {
    return m_layoutCounter;
}

std::shared_ptr<Widget> WidgetTree::root() const noexcept {
    return m_root;
}

void WidgetTree::detach(Widget* widget) {
    onDetached(widget);
}

void WidgetTree::attach(Widget* widget) {
    for (WidgetGroup* g : widget->m_groups) {
        addGroup(g);
    }
    onAttached(widget);
}

constexpr double refreshInterval = 0.1; // in seconds

void WidgetTree::updateAndPaint(Canvas& canvas) {
    if (!m_root)
        return;
    bindings->assign(frameStartTime, currentTime());
    for (WidgetGroup* g : m_groups) {
        g->beforeFrame();
    }

    if (frameStartTime >= m_refreshTime + refreshInterval) [[unlikely]] {
        for (WidgetGroup* g : m_groups) {
            g->beforeRefresh();
        }
        m_root->refreshTree();
        m_refreshTime = frameStartTime;
    }
    processAnimation();
    processRebuild();

    for (WidgetGroup* g : m_groups) {
        g->beforeLayout(m_root->isLayoutDirty());
    }

    m_root->updateLayout(viewportRectangle);

    if (m_updateGeometryRequested) {
        inputQueue->reset();
        m_root->updateGeometry();
        m_updateGeometryRequested = false;
    }
    inputQueue->processEvents();

    m_root->restyleIfRequested();

    for (WidgetGroup* g : m_groups) {
        g->beforeLayout(m_root->isLayoutDirty());
    }

    m_root->updateLayout(viewportRectangle);

    for (WidgetGroup* g : m_groups) {
        g->beforePaint();
    }

    // Paint the widgets per-layer
    // Clear the layer and push the root widget's drawable
    m_layer.clear();
    m_layer.push_back(m_root->drawable(viewportRectangle));
    do {
        std::vector<Drawable> layer;
        // Swap the contents of the current layer with the m_layer vector.
        // This allows us to process the current layer while m_layer gets populated with the next layer of
        // drawables.
        std::swap(layer, m_layer);
        for (const Drawable& d : layer) {
            d(canvas);
        }
        // This loop continues until m_layer is empty, which means all drawables have been painted.
        // If any drawables in the current layer trigger the addition of new drawables to m_layer,
        // the loop will process those new drawables in the subsequent iterations.
    } while (!m_layer.empty());

    if (Internal::debugBoundaries) {
        std::optional<Rectangle> rect = inputQueue->getAtMouse<Rectangle>([](Widget* w) {
            return w->rect();
        });
        if (rect) {
            canvas.raw().drawRectangle(*rect, 0.f, 0.f, fillColor = 0x102040'40_rgba, strokeWidth = 0.f);
        }
    }

    for (WidgetGroup* g : m_groups) {
        g->afterFrame();
    }
}

void WidgetTree::requestUpdateGeometry() {
    m_updateGeometryRequested = true;
}

void WidgetTree::addGroup(WidgetGroup* group) {
    m_groups.insert(group);
}

} // namespace Brisk
