#pragma once

#include "vsm/ui/WaveformSeries.h" // complete type required by moc for the Q_PROPERTY

#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

namespace vsm::ui {

/// Qt Quick item that draws a WaveformSeries as a monitor-style sweep.
///
/// Rendering uses the Qt Quick scene graph directly (one QSGGeometryNode with
/// line segments) instead of QPainter: the geometry is uploaded to the GPU and
/// only rebuilt when new samples arrive. A small gap after the write position
/// acts as the "erase bar" that separates new from old data.
class WaveformItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(vsm::ui::WaveformSeries* series READ series WRITE setSeries NOTIFY seriesChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    /// Value shown at the bottom / top edge (e.g. -1.0 .. 2.0 mV for ECG).
    Q_PROPERTY(double minimumValue READ minimumValue WRITE setMinimumValue NOTIFY rangeChanged)
    Q_PROPERTY(double maximumValue READ maximumValue WRITE setMaximumValue NOTIFY rangeChanged)
    Q_PROPERTY(double lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)

public:
    explicit WaveformItem(QQuickItem* parent = nullptr);

    [[nodiscard]] WaveformSeries* series() const { return m_series; }
    void setSeries(WaveformSeries* series);

    [[nodiscard]] QColor color() const { return m_color; }
    void setColor(const QColor& color);

    [[nodiscard]] double minimumValue() const noexcept { return m_minimumValue; }
    void setMinimumValue(double value);
    [[nodiscard]] double maximumValue() const noexcept { return m_maximumValue; }
    void setMaximumValue(double value);

    [[nodiscard]] double lineWidth() const noexcept { return m_lineWidth; }
    void setLineWidth(double width);

signals:
    void seriesChanged();
    void colorChanged();
    void rangeChanged();
    void lineWidthChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    QPointer<WaveformSeries> m_series;
    QMetaObject::Connection m_seriesConnection;
    QColor m_color{QStringLiteral("#00e676")};
    double m_minimumValue{-1.0};
    double m_maximumValue{2.0};
    double m_lineWidth{2.0};
};

} // namespace vsm::ui
