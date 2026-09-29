#include "WaveformWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <cmath>
#include <algorithm>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumHeight(125);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void WaveformWidget::setAudioData(const std::vector<float> &pcmSamples, qint64 durationMs) {
    m_pcmSamples = pcmSamples;
    m_durationMs = durationMs;
    m_positionMs = 0;
    m_zoomFactor = 1.0;
    m_scrollOffsetPx = 0;
    clearSelection();

    recomputePeaks();
    update();
}

void WaveformWidget::setLiveAudioData(const std::vector<float> &pcmSamples, qint64 durationMs) {
    m_pcmSamples = pcmSamples;
    m_durationMs = durationMs;
    m_positionMs = durationMs;
    clearSelection();

    recomputePeaks();
    update();
}

void WaveformWidget::setSegments(const QList<AudioSegment> &segments) {
    m_segments = segments;
    int wordCount = 0;
    for (const auto &seg : m_segments) {
        wordCount += seg.text.split(' ', Qt::SkipEmptyParts).size();
    }
    m_metrics = AudioUtils::analyzeSpeech(m_pcmSamples, m_durationMs, wordCount);
    update();
}

void WaveformWidget::setMode(WaveformMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        update();
    }
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
        m_pitchTrack.clear();
        m_energyTrack.clear();
        m_metrics = AudioUtils::SpeechMetrics();
        return;
    }

    int totalPixels = static_cast<int>(width() * m_zoomFactor);
    m_peaks = AudioUtils::computeWaveformPeaks(m_pcmSamples, totalPixels);
    m_pitchTrack = AudioUtils::computePitchTrack(m_pcmSamples);
    m_energyTrack = AudioUtils::computeEnergyTrack(m_pcmSamples);

    int wordCount = 0;
    for (const auto &seg : m_segments) {
        wordCount += seg.text.split(' ', Qt::SkipEmptyParts).size();
    }
    m_metrics = AudioUtils::analyzeSpeech(m_pcmSamples, m_durationMs, wordCount);
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

void WaveformWidget::setSelection(qint64 startMs, qint64 endMs) {
    if (startMs >= endMs || m_durationMs <= 0) {
        clearSelection();
        return;
    }
    m_selectionStartMs = std::max<qint64>(0, startMs);
    m_selectionEndMs = std::min<qint64>(m_durationMs, endMs);
    m_hasSelection = true;
    emit rangeSelected(m_selectionStartMs, m_selectionEndMs);
    update();
}

void WaveformWidget::clearSelection() {
    if (m_hasSelection) {
        m_hasSelection = false;
        m_selectionStartMs = 0;
        m_selectionEndMs = 0;
        emit selectionCleared();
        update();
    }
}

void WaveformWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && m_hasSelection) {
        clearSelection();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void WaveformWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        if (m_hasSelection) {
            clearSelection();
            return;
        }
    }

    if (event->button() == Qt::LeftButton) {
        m_isMouseDown = true;
        m_isDragging = false;
        m_dragStartPos = event->pos();

        // 1. Scrubbing Time Ruler at top
        if (event->pos().y() <= RULER_HEIGHT) {
            m_dragMode = DragMode::ScrubbingRuler;
            qint64 ms = xToMs(event->pos().x());
            emit seekRequested(ms);
            return;
        }

        // 2. Selection Handles / Inside Selection Detection
        if (m_hasSelection) {
            int leftX = msToX(m_selectionStartMs);
            int rightX = msToX(m_selectionEndMs);
            if (std::abs(event->pos().x() - leftX) <= 6) {
                m_dragMode = DragMode::ResizeSelectionLeft;
            } else if (std::abs(event->pos().x() - rightX) <= 6) {
                m_dragMode = DragMode::ResizeSelectionRight;
            } else if (event->pos().x() > leftX && event->pos().x() < rightX) {
                m_dragMode = DragMode::InsideSelection;
            } else {
                m_dragMode = DragMode::None;
            }
        } else {
            m_dragMode = DragMode::None;
        }
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent *event) {
    m_mouseX = event->pos().x();

    if (!m_isMouseDown) {
        // Dynamic cursor shape for handles and selection
        if (m_hasSelection) {
            int leftX = msToX(m_selectionStartMs);
            int rightX = msToX(m_selectionEndMs);
            if (std::abs(m_mouseX - leftX) <= 6 || std::abs(m_mouseX - rightX) <= 6) {
                setCursor(Qt::SizeHorCursor);
            } else if (m_mouseX > leftX && m_mouseX < rightX) {
                setCursor(Qt::PointingHandCursor);
            } else {
                setCursor(Qt::CrossCursor);
            }
        } else {
            setCursor(Qt::CrossCursor);
        }

        int segId = findSegmentAtX(m_mouseX);
        if (segId != m_hoveredSegmentId) {
            m_hoveredSegmentId = segId;
        }
        update();
        return;
    }

    // Scrubbing via Time Ruler
    if (m_dragMode == DragMode::ScrubbingRuler) {
        qint64 ms = xToMs(m_mouseX);
        emit seekRequested(ms);
        update();
        return;
    }

    // Detect drag threshold (5 pixels)
    int dx = event->pos().x() - m_dragStartPos.x();
    if (!m_isDragging && std::abs(dx) > 5) {
        m_isDragging = true;
        if (m_dragMode == DragMode::None || m_dragMode == DragMode::InsideSelection) {
            m_dragMode = DragMode::SelectingRange;
        }
    }

    if (m_isDragging) {
        if (m_dragMode == DragMode::SelectingRange) {
            qint64 t1 = xToMs(m_dragStartPos.x());
            qint64 t2 = xToMs(event->pos().x());
            m_selectionStartMs = std::max<qint64>(0, std::min(t1, t2));
            m_selectionEndMs = std::min<qint64>(m_durationMs, std::max(t1, t2));
            m_hasSelection = (m_selectionEndMs > m_selectionStartMs + 25);
            if (m_hasSelection) {
                emit rangeSelected(m_selectionStartMs, m_selectionEndMs);
            }
            update();
        } else if (m_dragMode == DragMode::ResizeSelectionLeft) {
            qint64 newStart = xToMs(event->pos().x());
            newStart = std::max<qint64>(0, std::min(newStart, m_selectionEndMs - 25));
            m_selectionStartMs = newStart;
            emit rangeSelected(m_selectionStartMs, m_selectionEndMs);
            update();
        } else if (m_dragMode == DragMode::ResizeSelectionRight) {
            qint64 newEnd = xToMs(event->pos().x());
            newEnd = std::min<qint64>(m_durationMs, std::max(newEnd, m_selectionStartMs + 25));
            m_selectionEndMs = newEnd;
            emit rangeSelected(m_selectionStartMs, m_selectionEndMs);
            update();
        }
    }
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_dragMode == DragMode::ScrubbingRuler) {
            m_dragMode = DragMode::None;
            m_isMouseDown = false;
            m_isDragging = false;
            return;
        }

        if (m_isDragging) {
            // Drag completed
            if (m_hasSelection && (m_selectionEndMs - m_selectionStartMs >= 25)) {
                emit rangeSelected(m_selectionStartMs, m_selectionEndMs);
            } else {
                clearSelection();
            }
        } else {
            // Pure Click (not a drag)
            if (m_hasSelection && m_dragMode == DragMode::InsideSelection) {
                // Replay selected range!
                emit playRangeRequested(m_selectionStartMs, m_selectionEndMs);
            } else {
                if (m_hasSelection) {
                    clearSelection();
                }

                qint64 clickedMs = xToMs(event->pos().x());
                int segId = findSegmentAtX(event->pos().x());
                if (segId != -1) {
                    for (const auto &seg : m_segments) {
                        if (seg.id == segId) {
                            emit segmentClicked(seg.id, seg.startMs, seg.endMs);
                            break;
                        }
                    }
                }

                switch (m_clickMode) {
                    case ClickMode::PlayFromClick:
                        emit seekRequested(clickedMs);
                        emit playRequested(clickedMs);
                        break;
                    case ClickMode::PreviewSnippet:
                        emit seekRequested(clickedMs);
                        emit previewRequested(clickedMs, m_previewDurationMs);
                        break;
                    case ClickMode::SeekOnly:
                        emit seekRequested(clickedMs);
                        break;
                }
            }
        }

        m_isMouseDown = false;
        m_isDragging = false;
        m_dragMode = DragMode::None;
        update();
    }
}

void WaveformWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int segId = findSegmentAtX(event->pos().x());
        if (segId != -1) {
            for (const auto &seg : m_segments) {
                if (seg.id == segId) {
                    emit segmentDoubleClicked(seg.id, seg.startMs, seg.endMs);
                    return;
                }
            }
        }

        if (m_hasSelection) {
            emit playRangeRequested(m_selectionStartMs, m_selectionEndMs);
            return;
        }

        qint64 clickedMs = xToMs(event->pos().x());
        emit seekRequested(clickedMs);
        emit playRequested(clickedMs);
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
    p.fillRect(0, 0, w, h, QColor(20, 22, 28));

    // 2. Time Ruler Background
    p.fillRect(0, 0, w, RULER_HEIGHT, QColor(28, 30, 38));
    p.setPen(QColor(45, 48, 60));
    p.drawLine(0, RULER_HEIGHT, w, RULER_HEIGHT);

    // Time Ruler Ticks & Labels
    if (m_durationMs > 0) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont rulerFont = font();
        rulerFont.setPointSize(8);
        p.setFont(rulerFont);

        double totalSec = m_durationMs / 1000.0;
        double visibleSec = totalSec / m_zoomFactor;
        double stepSec = 1.0;
        if (visibleSec > 60) stepSec = 10.0;
        else if (visibleSec > 30) stepSec = 5.0;
        else if (visibleSec > 10) stepSec = 2.0;

        for (double sec = 0.0; sec <= totalSec; sec += stepSec) {
            int x = msToX(static_cast<qint64>(sec * 1000));
            if (x < -50 || x > w + 50) continue;

            p.setPen(QColor(90, 98, 115));
            p.drawLine(x, RULER_HEIGHT - 6, x, RULER_HEIGHT);

            int minutes = static_cast<int>(sec) / 60;
            int seconds = static_cast<int>(sec) % 60;
            QString label = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
            p.setPen(QColor(150, 158, 175));
            p.drawText(x + 2, RULER_HEIGHT - 8, label);
        }
    }

    // 3. Audio Visualization Body (Multi-mode)
    int waveTop = RULER_HEIGHT;
    int waveHeight = h - waveTop;
    int waveCenterY = waveTop + waveHeight / 2;

    if (!m_peaks.empty()) {
        int totalPixels = static_cast<int>(width() * m_zoomFactor);

        // A. Amplitude peaks (for Waveform & CombinedStudio modes)
        if (m_mode == WaveformMode::Waveform || m_mode == WaveformMode::CombinedStudio) {
            p.setPen(QColor(38, 42, 54));
            p.drawLine(0, waveCenterY, w, waveCenterY);

            int totalPeaks = m_peaks.size();
            float halfH = (waveHeight / 2.0f) * 0.88f;
            QColor waveColor = (m_mode == WaveformMode::Waveform)
                ? QColor(0, 190, 230) // vibrant cyan
                : QColor(0, 160, 210, 100); // translucent cyan for master combined mode

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
                    p.setPen(waveColor);
                    p.drawLine(x, yMin, x, yMax);
                }
            }
        }

        // B. Speech Energy Envelope (in EnergyEnvelope mode)
        if (m_mode == WaveformMode::EnergyEnvelope && !m_energyTrack.empty()) {
            p.setRenderHint(QPainter::Antialiasing, false);
            for (int x = 0; x < w; ++x) {
                int realX = x + m_scrollOffsetPx;
                if (totalPixels > 0) {
                    size_t frameIdx = static_cast<size_t>((static_cast<double>(realX) / totalPixels) * m_energyTrack.size());
                    if (frameIdx < m_energyTrack.size()) {
                        float energy = m_energyTrack[frameIdx];
                        int barH = static_cast<int>(energy * (waveHeight - 20));
                        int yTop = waveTop + waveHeight - barH - 4;
                        int yBot = waveTop + waveHeight - 4;

                        QColor col = (energy > 0.6f) ? QColor(249, 115, 22)
                                   : ((energy > 0.25f) ? QColor(6, 182, 212)
                                                       : QColor(79, 70, 229));
                        p.setPen(col);
                        p.drawLine(x, yTop, x, yBot);
                    }
                }
            }
        }

        // C. Pitch & Intonation Track (in PitchContour & CombinedStudio modes)
        if ((m_mode == WaveformMode::PitchContour || m_mode == WaveformMode::CombinedStudio) && !m_pitchTrack.empty()) {
            p.setRenderHint(QPainter::Antialiasing, true);

            // Pitch reference lines in PitchContour mode
            if (m_mode == WaveformMode::PitchContour) {
                QFont gridFont = font();
                gridFont.setPointSize(7);
                p.setFont(gridFont);
                int refHz[] = {100, 150, 200, 250, 300};
                for (int hz : refHz) {
                    float norm = (hz - 65.0f) / (350.0f - 65.0f);
                    int y = waveTop + waveHeight - static_cast<int>(norm * (waveHeight - 24)) - 10;
                    p.setPen(QColor(40, 46, 62));
                    p.drawLine(0, y, w, y);
                    p.setPen(QColor(100, 116, 139));
                    p.drawText(w - 38, y - 2, QString("%1Hz").arg(hz));
                }
            }

            QPainterPath pitchPath;
            bool inPath = false;
            float minF0 = 65.0f;
            float maxF0 = 350.0f;

            QPen pitchPen = (m_mode == WaveformMode::PitchContour)
                ? QPen(QColor(251, 191, 36), 2.5) // Electric Yellow
                : QPen(QColor(245, 158, 11), 2.0); // Golden Amber for Combined

            for (size_t f = 0; f < m_pitchTrack.size(); ++f) {
                float f0 = m_pitchTrack[f];
                if (f0 > 55.0f) {
                    double progress = static_cast<double>(f) / m_pitchTrack.size();
                    int x = static_cast<int>(progress * totalPixels) - m_scrollOffsetPx;
                    float norm = std::clamp((f0 - minF0) / (maxF0 - minF0), 0.0f, 1.0f);
                    int y = waveTop + waveHeight - static_cast<int>(norm * (waveHeight - 24)) - 10;

                    if (!inPath) {
                        pitchPath.moveTo(x, y);
                        inPath = true;
                    } else {
                        pitchPath.lineTo(x, y);
                    }
                } else {
                    inPath = false;
                }
            }

            p.strokePath(pitchPath, pitchPen);
        }
    } else {
        // Empty state message
        p.setPen(QColor(100, 105, 120));
        QFont emptyFont = font();
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect().adjusted(0, RULER_HEIGHT, 0, 0), Qt::AlignCenter,
                   tr("No audio loaded. Record or open a session to view waveform & speech intonation."));
    }

    // 4. Pronunciation & Intonation HUD Bar
    if (m_durationMs > 0 && m_metrics.meanPitchHz > 0) {
        p.setRenderHint(QPainter::Antialiasing, true);
        QFont hudFont = font();
        hudFont.setPointSize(8);
        hudFont.setBold(true);
        p.setFont(hudFont);

        QString hudText = QString(
            "🎵 Pitch: %1 Hz (%2-%3 Hz)  •  Intonation: %4  •  🗣️ Tempo: %5 WPM (%6)  •  Speech: %7% | Pauses: %8%  •  Peak: %9 dBFS"
        )
        .arg(static_cast<int>(m_metrics.meanPitchHz))
        .arg(static_cast<int>(m_metrics.minPitchHz))
        .arg(static_cast<int>(m_metrics.maxPitchHz))
        .arg(m_metrics.intonationTrend)
        .arg(m_metrics.wordsPerMinute)
        .arg(m_metrics.tempoRating)
        .arg(static_cast<int>(m_metrics.speechRatioPercent))
        .arg(static_cast<int>(m_metrics.pauseRatioPercent))
        .arg(m_metrics.peakDbfs, 0, 'f', 1);

        QFontMetrics fm(hudFont);
        int hudW = fm.horizontalAdvance(hudText) + 16;
        int hudH = 18;
        int hudX = std::max(6, (w - hudW) / 2);
        int hudY = h - hudH - 3;

        p.setBrush(QColor(15, 18, 28, 225));
        p.setPen(QPen(QColor(45, 54, 78), 1));
        p.drawRoundedRect(hudX, hudY, hudW, hudH, 4, 4);

        p.setPen(QColor(226, 232, 240));
        p.drawText(hudX, hudY, hudW, hudH, Qt::AlignCenter, hudText);
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

    // 5. Active Range Selection Overlay
    if (m_hasSelection && m_durationMs > 0) {
        int x0 = msToX(m_selectionStartMs);
        int x1 = msToX(m_selectionEndMs);
        int selW = std::max(2, x1 - x0);

        if (x1 >= 0 && x0 <= w) {
            p.setRenderHint(QPainter::Antialiasing, true);

            // Shaded range background
            p.fillRect(x0, waveTop, selW, waveHeight, QColor(245, 158, 11, 45));

            // Top and bottom borders
            p.setPen(QPen(QColor(245, 158, 11, 140), 1));
            p.drawLine(x0, waveTop, x1, waveTop);
            p.drawLine(x0, h - 1, x1, h - 1);

            // Left & Right boundary lines
            p.setPen(QPen(QColor(245, 158, 11, 230), 2));
            p.drawLine(x0, waveTop, x0, h);
            p.drawLine(x1, waveTop, x1, h);

            // Boundary grab handles
            p.setBrush(QColor(245, 158, 11));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(x0 - 3, waveTop + 3, 6, 14, 2, 2);
            p.drawRoundedRect(x1 - 3, waveTop + 3, 6, 14, 2, 2);

            // Duration Pill Badge
            double durSec = (m_selectionEndMs - m_selectionStartMs) / 1000.0;
            QString badgeText = QString("▶ %1s").arg(durSec, 0, 'f', 2);
            QFont badgeFont = font();
            badgeFont.setPointSize(8);
            badgeFont.setBold(true);
            p.setFont(badgeFont);

            QFontMetrics fm(badgeFont);
            int bw = fm.horizontalAdvance(badgeText) + 14;
            int bh = 18;
            int bx = std::max(x0 + 2, std::min(x1 - bw - 2, (x0 + x1 - bw) / 2));
            int by = waveTop + 4;

            p.setBrush(QColor(217, 119, 6, 230));
            p.setPen(QPen(QColor(251, 191, 36), 1));
            p.drawRoundedRect(bx, by, bw, bh, 4, 4);

            p.setPen(Qt::white);
            p.drawText(bx, by, bw, bh, Qt::AlignCenter, badgeText);
        }
    }

    // 6. Playhead Line and Badge
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

    // 7. Hover cursor line
    if (m_mouseX >= 0 && m_mouseX <= w && !m_isMouseDown) {
        p.setPen(QPen(QColor(255, 255, 255, 60), 1, Qt::DashLine));
        p.drawLine(m_mouseX, 0, m_mouseX, h);
    }
}
