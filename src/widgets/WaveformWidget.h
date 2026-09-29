#pragma once

#include <QWidget>
#include <QVector>
#include <QList>
#include <vector>
#include "../audio/AudioUtils.h"
#include "../models/SessionData.h"

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr);
    ~WaveformWidget() override = default;

    void setAudioData(const std::vector<float> &pcmSamples, qint64 durationMs);
    void setSegments(const QList<AudioSegment> &segments);
    void setPlaybackPosition(qint64 positionMs);
    void setSelectedSegmentId(int segmentId);

    qint64 duration() const { return m_durationMs; }
    qint64 position() const { return m_positionMs; }
    double zoomFactor() const { return m_zoomFactor; }

public slots:
    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void seekRequested(qint64 positionMs);
    void segmentClicked(int segmentId, qint64 startMs, qint64 endMs);
    void segmentDoubleClicked(int segmentId, qint64 startMs, qint64 endMs);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void recomputePeaks();
    int msToX(qint64 ms) const;
    qint64 xToMs(int x) const;
    int findSegmentAtX(int x) const;

    std::vector<float> m_pcmSamples;
    QVector<AudioUtils::WaveformPeak> m_peaks;
    QList<AudioSegment> m_segments;

    qint64 m_durationMs = 0;
    qint64 m_positionMs = 0;
    int m_selectedSegmentId = -1;
    int m_hoveredSegmentId = -1;

    double m_zoomFactor = 1.0;
    int m_scrollOffsetPx = 0;

    bool m_isDraggingPlayhead = false;
    int m_mouseX = -1;

    const int RULER_HEIGHT = 22;
};
