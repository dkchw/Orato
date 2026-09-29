#include "WaveformWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <cmath>
#include <algorithm>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumHeight(110);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void WaveformWidget::setAudioData(const std::vector<float> &pcmSamples, qint64 durationMs) {
    m_pcmSamples = pcmSamples;
    m_durationMs = durationMs;
    m_positionMs = 0;
    m_zoomFactor = 1.0;
    m_scrollOffsetPx = 0;

    recomputePeaks();
    update();
}

void WaveformWidget::setLiveAudioData(const std::vector<float> &pcmSamples, qint64 durationMs) {
    m_pcmSamples = pcmSamples;
    m_durationMs = durationMs;
    m_positionMs = durationMs;

    recomputePeaks();
    update();
}

void WaveformWidget::setSegments(const QList<AudioSegment> &segments) {
    m_segments = segments;
    update();
}

void WaveformWidget::setPlaybackPosition(qint64 positionMs) {
    m_positionMs = positionMs;

    // Active segment detection
    int activeId = -1;
    for (const auto &seg : m_segments) {
        if (positionMs >= seg.startMs && positionMs <= seg.endMs) {
            activeId = seg.id;
            break;
        }
    }
    if (activeId != m_selectedSegmentId) {
        m_selectedSegmentId = activeId;
    }

    update();
}

void WaveformWidget::setSelectedSegmentId(int segmentId) {
    m_selectedSegmentId = segmentId;
    update();
}

void WaveformWidget::zoomIn() {
    m_zoomFactor = std::min(10.0, m_zoomFactor * 1.3);
    recomputePeaks();
    update();
}

void WaveformWidget::zoomOut() {
    m_zoomFactor = std::max(1.0, m_zoomFactor / 1.3);
    recomputePeaks();
    update();
}

void WaveformWidget::zoomFit() {
    m_zoomFactor = 1.0;
    m_scrollOffsetPx = 0;
    recomputePeaks();
    update();
}

void WaveformWidget::resizeEvent(QResizeEvent *) {
    recomputePeaks();
}

void WaveformWidget::recomputePeaks() {
    if (m_pcmSamples.empty() || width() <= 0) {
        m_peaks.clear();
        return;
    }

    int totalPixels = static_cast<int>(width() * m_zoomFactor);
    m_peaks = AudioUtils::computeWaveformPeaks(m_pcmSamples, totalPixels);
}

int WaveformWidget::msToX(qint64 ms) const {
    if (m_durationMs <= 0) return 0;
    double progress = static_cast<double>(ms) / static_cast<double>(m_durationMs);
    int totalWidth = static_cast<int>(width() * m_zoomFactor);
    return static_cast<int>(progress * totalWidth) - m_scrollOffsetPx;
}

qint64 WaveformWidget::xToMs(int x) const {
    if (m_durationMs <= 0 || width() <= 0) return 0;
    int totalWidth = static_cast<int>(width() * m_zoomFactor);
    int realX = x + m_scrollOffsetPx;
    double progress = std::max(0.0, std::min(1.0, static_cast<double>(realX) / totalWidth));
    return static_cast<qint64>(progress * m_durationMs);
}

int WaveformWidget::findSegmentAtX(int x) const {
    qint64 ms = xToMs(x);
    for (const auto &seg : m_segments) {
        if (ms >= seg.startMs && ms <= seg.endMs) {
            return seg.id;
        }
    }
    return -1;
}

void WaveformWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDraggingPlayhead = true;
        qint64 ms = xToMs(event->pos().x());
        emit seekRequested(ms);

        int segId = findSegmentAtX(event->pos().x());
        if (segId != -1) {
            for (const auto &seg : m_segments) {
                if (seg.id == segId) {
                    emit segmentClicked(seg.id, seg.startMs, seg.endMs);
                    break;
                }
            }
        }
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent *event) {
    m_mouseX = event->pos().x();

    if (m_isDraggingPlayhead) {
        qint64 ms = xToMs(m_mouseX);
        emit seekRequested(ms);
    }

    int segId = findSegmentAtX(m_mouseX);
    if (segId != m_hoveredSegmentId) {
        m_hoveredSegmentId = segId;
    }
    update();
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDraggingPlayhead = false;
    }
}

void WaveformWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int segId = findSegmentAtX(event->pos().x());
        if (segId != -1) {
            for (const auto &seg : m_segments) {
                if (seg.id == segId) {
                    emit segmentDoubleClicked(seg.id, seg.startMs, seg.endMs);
                    break;
                }
            }
        }
    }
}

void WaveformWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void WaveformWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int w = width();
    int h = height();

    // 1. Background
    p.fillRect(0, 0, w, h, QColor(24, 25, 30));

    // 2. Time Ruler Background
    p.fillRect(0, 0, w, RULER_HEIGHT, QColor(32, 34, 42));
    p.setPen(QColor(50, 54, 66));
    p.drawLine(0, RULER_HEIGHT, w, RULER_HEIGHT);

    // Time Ruler Ticks & Labels
    if (m_durationMs > 0) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont rulerFont = font();
        rulerFont.setPointSize(8);
        p.setFont(rulerFont);

        // Determine sensible time intervals depending on duration and zoom
        double totalSec = m_durationMs / 1000.0;
        double visibleSec = totalSec / m_zoomFactor;
        double stepSec = 1.0;
        if (visibleSec > 60) stepSec = 10.0;
        else if (visibleSec > 30) stepSec = 5.0;
        else if (visibleSec > 10) stepSec = 2.0;

        for (double sec = 0.0; sec <= totalSec; sec += stepSec) {
            int x = msToX(static_cast<qint64>(sec * 1000));
            if (x < -50 || x > w + 50) continue;

            p.setPen(QColor(100, 108, 125));
            p.drawLine(x, RULER_HEIGHT - 6, x, RULER_HEIGHT);

            int minutes = static_cast<int>(sec) / 60;
            int seconds = static_cast<int>(sec) % 60;
            QString label = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
            p.setPen(QColor(160, 168, 185));
            p.drawText(x + 2, RULER_HEIGHT - 8, label);
        }
    }

    // 3. Audio Waveform Body
    int waveTop = RULER_HEIGHT;
    int waveHeight = h - waveTop;
    int waveCenterY = waveTop + waveHeight / 2;

    // Draw center zero line
    p.setPen(QColor(40, 44, 54));
    p.drawLine(0, waveCenterY, w, waveCenterY);

    if (!m_peaks.empty()) {
        int totalPeaks = m_peaks.size();
        float halfH = (waveHeight / 2.0f) * 0.9f;

        for (int x = 0; x < w; ++x) {
            int peakIdx = x + m_scrollOffsetPx;
            if (peakIdx >= 0 && peakIdx < totalPeaks) {
                const auto &pk = m_peaks[peakIdx];
                int yMin = waveCenterY - static_cast<int>(pk.maxVal * halfH);
                int yMax = waveCenterY - static_cast<int>(pk.minVal * halfH);
                if (yMin == yMax) {
                    yMin -= 1;
                    yMax += 1;
                }

                p.setPen(QColor(0, 180, 216)); // bright cyan
                p.drawLine(x, yMin, x, yMax);
            }
        }
    } else {
        // Empty state message
        p.setPen(QColor(100, 105, 120));
        QFont emptyFont = font();
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect().adjusted(0, RULER_HEIGHT, 0, 0), Qt::AlignCenter,
                   tr("No audio loaded. Record or open a session to view waveform."));
    }

    // 4. Segments Overlay
    p.setRenderHint(QPainter::Antialiasing, true);
    for (const auto &seg : m_segments) {
        int x0 = msToX(seg.startMs);
        int x1 = msToX(seg.endMs);
        int segW = std::max(4, x1 - x0);

        if (x1 < 0 || x0 > w) continue;

        bool isSelected = (seg.id == m_selectedSegmentId);
        bool isHovered = (seg.id == m_hoveredSegmentId);

        QColor fillColor = isSelected ? QColor(46, 204, 113, 70)
                        : (isHovered ? QColor(52, 152, 219, 60)
                                     : QColor(52, 152, 219, 25));
        QColor borderColor = isSelected ? QColor(46, 204, 113, 200) : QColor(52, 152, 219, 120);

        p.fillRect(x0, waveTop, segW, waveHeight, fillColor);
        p.setPen(QPen(borderColor, isSelected ? 2 : 1));
        p.drawRect(x0, waveTop, segW, waveHeight);

        // Segment badge number
        QFont badgeFont = font();
        badgeFont.setPointSize(7);
        badgeFont.setBold(true);
        p.setFont(badgeFont);

        QString badge = QString("#%1").arg(seg.id);
        p.setPen(isSelected ? QColor(46, 204, 113) : QColor(140, 190, 240));
        p.drawText(x0 + 4, waveTop + 14, badge);
    }

    // 5. Playhead Line and Badge
    if (m_durationMs > 0) {
        int playheadX = msToX(m_positionMs);
        if (playheadX >= 0 && playheadX <= w) {
            // Vertical playhead line
            p.setPen(QPen(QColor(255, 71, 87), 2));
            p.drawLine(playheadX, 0, playheadX, h);

            // Top Playhead Badge
            int totalSec = static_cast<int>(m_positionMs / 1000);
            int minutes = totalSec / 60;
            int seconds = totalSec % 60;
            int tenths = static_cast<int>((m_positionMs % 1000) / 100);
            QString timeStr = QString("%1:%2.%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(tenths);

            QFont badgeFont = font();
            badgeFont.setPointSize(8);
            badgeFont.setBold(true);
            p.setFont(badgeFont);

            QFontMetrics fm(badgeFont);
            int badgeW = fm.horizontalAdvance(timeStr) + 8;
            int badgeH = 16;
            int badgeX = std::max(2, std::min(w - badgeW - 2, playheadX - badgeW / 2));

            p.setBrush(QColor(255, 71, 87));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(badgeX, 2, badgeW, badgeH, 3, 3);

            p.setPen(Qt::white);
            p.drawText(badgeX, 2, badgeW, badgeH, Qt::AlignCenter, timeStr);
        }
    }

    // 6. Hover cursor line
    if (m_mouseX >= 0 && m_mouseX <= w && !m_isDraggingPlayhead) {
        p.setPen(QPen(QColor(255, 255, 255, 60), 1, Qt::DashLine));
        p.drawLine(m_mouseX, 0, m_mouseX, h);
    }
}
