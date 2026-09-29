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

    enum class WaveformMode {
        Waveform,       // Classic amplitude waveform
        PitchContour,   // F0 Intonation melody contour (Hz)
        EnergyEnvelope, // Speech volume dynamics & stress (dB)
        CombinedStudio  // Amplitude waveform + Pitch intonation overlay
    };

    enum class ClickMode {
        PlayFromClick,   // Start continuous playback from clicked timestamp
        PreviewSnippet,  // Play short preview snippet (e.g. 2.5s) from clicked timestamp
        SeekOnly         // Seek playhead only
    };

    enum class DragMode {
        None,
        ScrubbingRuler,
        SelectingRange,
        ResizeSelectionLeft,
        ResizeSelectionRight,
        InsideSelection
    };

    void setAudioData(const std::vector<float> &pcmSamples, qint64 durationMs);
    void setLiveAudioData(const std::vector<float> &pcmSamples, qint64 durationMs);
    void setSegments(const QList<AudioSegment> &segments);
    void setPlaybackPosition(qint64 positionMs);
    void setSelectedSegmentId(int segmentId);
    void setMode(WaveformMode mode);
    WaveformMode mode() const { return m_mode; }
    const AudioUtils::SpeechMetrics& speechMetrics() const { return m_metrics; }

    void setClickMode(ClickMode mode) { m_clickMode = mode; }
    ClickMode clickMode() const { return m_clickMode; }

    void setPreviewDuration(qint64 durationMs) { m_previewDurationMs = durationMs; }
    qint64 previewDuration() const { return m_previewDurationMs; }

    void setSelection(qint64 startMs, qint64 endMs);
    void clearSelection();
    bool hasSelection() const { return m_hasSelection; }
    qint64 selectionStart() const { return m_selectionStartMs; }
    qint64 selectionEnd() const { return m_selectionEndMs; }

    qint64 duration() const { return m_durationMs; }
    qint64 position() const { return m_positionMs; }
    double zoomFactor() const { return m_zoomFactor; }

public slots:
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void setWaveformMode(WaveformMode mode) { setMode(mode); }

signals:
    void seekRequested(qint64 positionMs);
    void playRequested(qint64 positionMs);
    void previewRequested(qint64 positionMs, qint64 durationMs);
    void rangeSelected(qint64 startMs, qint64 endMs);
    void selectionCleared();
    void playRangeRequested(qint64 startMs, qint64 endMs);
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
    void keyPressEvent(QKeyEvent *event) override;

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

    int m_mouseX = -1;

    WaveformMode m_mode = WaveformMode::CombinedStudio;
    ClickMode m_clickMode = ClickMode::PlayFromClick;
    qint64 m_previewDurationMs = 2500;

    DragMode m_dragMode = DragMode::None;
    bool m_isMouseDown = false;
    bool m_isDragging = false;
    QPoint m_dragStartPos;

    bool m_hasSelection = false;
    qint64 m_selectionStartMs = 0;
    qint64 m_selectionEndMs = 0;

    std::vector<float> m_pitchTrack;
    std::vector<float> m_energyTrack;
    AudioUtils::SpeechMetrics m_metrics;

    const int RULER_HEIGHT = 22;
};
