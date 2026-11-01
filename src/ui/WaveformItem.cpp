#include "vsm/ui/WaveformItem.h"

#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRendererInterface>
#include <QQuickWindow>

#include <algorithm>

namespace vsm::ui {

namespace {

/// Width of the erase bar in samples, as a fraction of the sweep.
constexpr std::size_t kGapFraction = 50; // 2 % of the screen width

} // namespace

WaveformItem::WaveformItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void WaveformItem::setSeries(WaveformSeries* series)
{
    if (m_series == series) {
        return;
    }
    disconnect(m_seriesConnection);
    m_series = series;
    if (m_series) {
        m_seriesConnection = connect(m_series, &WaveformSeries::samplesAppended, this, &QQuickItem::update);
    }
    emit seriesChanged();
    update();
}

void WaveformItem::setColor(const QColor& color)
{
    if (m_color == color) {
        return;
    }
    m_color = color;
    emit colorChanged();
    update();
}

void WaveformItem::setMinimumValue(double value)
{
    if (qFuzzyCompare(m_minimumValue, value)) {
        return;
    }
    m_minimumValue = value;
    emit rangeChanged();
    update();
}

void WaveformItem::setMaximumValue(double value)
{
    if (qFuzzyCompare(m_maximumValue, value)) {
        return;
    }
    m_maximumValue = value;
    emit rangeChanged();
    update();
}

void WaveformItem::setLineWidth(double width)
{
    if (qFuzzyCompare(m_lineWidth, width)) {
        return;
    }
    m_lineWidth = width;
    emit lineWidthChanged();
    update();
}

QSGNode* WaveformItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* /*data*/)
{
    auto* node = static_cast<QSGGeometryNode*>(oldNode);
    if (node == nullptr) {
        node = new QSGGeometryNode;
        auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
        geometry->setDrawingMode(QSGGeometry::DrawLines);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        auto* material = new QSGFlatColorMaterial;
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    }

    auto* material = static_cast<QSGFlatColorMaterial*>(node->material());
    if (material->color() != m_color) {
        material->setColor(m_color);
        node->markDirty(QSGNode::DirtyMaterial);
    }

    QSGGeometry* geometry = node->geometry();
    // Wide lines are only supported by OpenGL; Direct3D/Vulkan/Metal always
    // draw 1 px and log a warning per frame if asked for more. Thick lines on
    // every backend need triangle-strip geometry (planned improvement).
    const bool wideLinesSupported =
        window() != nullptr && window()->rendererInterface()->graphicsApi() == QSGRendererInterface::OpenGL;
    geometry->setLineWidth(wideLinesSupported ? static_cast<float>(m_lineWidth) : 1.0F);

    if (!m_series || width() <= 0.0 || height() <= 0.0 || m_maximumValue <= m_minimumValue) {
        geometry->allocate(0);
        node->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

    const auto& buffer = m_series->buffer();
    const auto samples = buffer.samples();
    const std::size_t count = buffer.capacity();
    const std::size_t filled = buffer.filledCount();
    const std::size_t writeIndex = buffer.writeIndex();
    const std::size_t gap = std::max<std::size_t>(1, count / kGapFraction);

    // A sample is visible if it holds data and is not inside the erase bar
    // that starts at the write position.
    const auto isVisible = [&](std::size_t index) {
        if (!buffer.isFull() && index >= filled) {
            return false;
        }
        const std::size_t distanceAfterWrite = (index + count - writeIndex) % count;
        return distanceAfterWrite >= gap;
    };

    const auto xScale = width() / static_cast<double>(count - 1);
    const auto yScale = height() / (m_maximumValue - m_minimumValue);
    const auto toY = [&](float value) {
        const double clamped = std::clamp(static_cast<double>(value), m_minimumValue, m_maximumValue);
        return static_cast<float>(height() - (clamped - m_minimumValue) * yScale);
    };

    const auto segmentVisible = [&](std::size_t i) { return isVisible(i) && isVisible(i + 1); };

    // Pass 1: count visible segments - QSGGeometry::allocate() discards the
    // old vertex data, so the exact size must be known before writing.
    int segments = 0;
    for (std::size_t i = 0; i + 1 < count; ++i) {
        segments += segmentVisible(i) ? 1 : 0;
    }

    // Pass 2: two vertices per segment (DrawLines).
    geometry->allocate(2 * segments);
    QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
    int used = 0;
    for (std::size_t i = 0; i + 1 < count; ++i) {
        if (!segmentVisible(i)) {
            continue;
        }
        vertices[used++].set(static_cast<float>(static_cast<double>(i) * xScale), toY(samples[i]));
        vertices[used++].set(static_cast<float>(static_cast<double>(i + 1) * xScale), toY(samples[i + 1]));
    }
    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}

} // namespace vsm::ui
