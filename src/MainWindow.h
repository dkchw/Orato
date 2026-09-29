#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QSlider>
#include <QTabWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QSplitter>

#include "audio/AudioRecorder.h"
#include "audio/AudioPlayer.h"
#include "whisper/WhisperEngine.h"
#include "whisper/ModelManager.h"
#include "tts/PocketTTSEngine.h"
#include "session/SessionManager.h"
#include "widgets/AudioLevelMeter.h"
#include "widgets/WaveformWidget.h"
#include "widgets/SentenceListView.h"
#include "widgets/MarkdownNoteEditor.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Session management
    void onNewSession();
    void onSaveSession();
    void onDeleteSession();
    void onSessionSelected(int index);
    void updateSessionList();

    // Audio recording
    void onRecordToggle();
    void onPauseToggle();
    void onRecordingDurationChanged(qint64 ms);
    void onRecordingFinished(const QString &savedPath, qint64 durationMs);
    void onAudioInputDeviceChanged(int index);
    void refreshAudioDevices();

    // Audio playback
    void onPlayToggle();
    void onStop();
    void onPlaybackPositionChanged(qint64 ms);
    void onPlaybackDurationChanged(qint64 ms);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onSpeedChanged(int index);
    void onVolumeSliderChanged(int value);
    void onMuteToggle();
    void onReplayCurrentSentence();
    void onLoopSentenceToggle(bool checked);

    // Whisper STT
    void onTranscribeClicked();
    void onWhisperStarted();
    void onWhisperProgress(int percent);
    void onWhisperSegmentDiscovered(const AudioSegment &seg);
    void onWhisperCompleted(const QList<AudioSegment> &segments);
    void onWhisperFailed(const QString &error);
    void onModelSelectionChanged(int index);
    void refreshModelList();

    // Pocket TTS
    void onTtsSentenceRequested(int id, const QString &text);
    void onTtsStudioGenerate();
    void onTtsStarted(const QString &text);
    void onTtsCompleted(const QString &outputPath);
    void onTtsFailed(const QString &error);

    // Timeline and Note coordination
    void onTimelineSeekRequested(qint64 ms);
    void onTimelineSegmentClicked(int segmentId, qint64 startMs, qint64 endMs);
    void onTimelineSegmentDoubleClicked(int segmentId, qint64 startMs, qint64 endMs);
    void onNoteTimestampClicked(qint64 ms);

    // Model Manager UI
    void onDownloadPresetRequested(const QString &presetId);
    void onAddCustomModelClicked();
    void onConvertHfModelClicked();

private:
    void setupUi();
    void setupTopBar(QWidget *container);
    void setupWaveformArea(QWidget *container);
    void setupBottomBar(QWidget *container);
    void setupSidebar(QWidget *container);
    void setupTtsStudioTab(QWidget *container);
    void setupModelManagerTab(QWidget *container);

    QString formatTime(qint64 ms) const;
    void loadCurrentSessionData();

    // Core managers
    AudioRecorder *m_recorder = nullptr;
    AudioPlayer *m_player = nullptr;
    WhisperEngine *m_whisper = nullptr;
    ModelManager *m_modelManager = nullptr;
    PocketTTSEngine *m_tts = nullptr;
    SessionManager *m_sessionManager = nullptr;

    SessionData m_currentSession;
    std::vector<float> m_currentAudioPcm;
    int m_activeSentenceId = -1;
    int m_requestingTtsSentenceId = -1;

    // UI Widgets
    QComboBox *m_inputDeviceCombo = nullptr;
    QPushButton *m_recordBtn = nullptr;
    QPushButton *m_pauseBtn = nullptr;
    QLabel *m_recordingTimeLabel = nullptr;
    AudioLevelMeter *m_levelMeter = nullptr;

    QComboBox *m_modelCombo = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QPushButton *m_transcribeBtn = nullptr;
    QProgressBar *m_transcribeProgress = nullptr;

    WaveformWidget *m_waveformWidget = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    SentenceListView *m_sentenceListView = nullptr;
    MarkdownNoteEditor *m_noteEditor = nullptr;

    // Sidebar
    QListWidget *m_sessionListWidget = nullptr;
    QLineEdit *m_sessionTitleEdit = nullptr;

    // Playback Controls
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_replaySentenceBtn = nullptr;
    QPushButton *m_loopSentenceBtn = nullptr;
    QPushButton *m_skipBackBtn = nullptr;
    QPushButton *m_skipForwardBtn = nullptr;
    QSlider *m_timelineSlider = nullptr;
    QLabel *m_playbackTimeLabel = nullptr;
    QComboBox *m_speedCombo = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QPushButton *m_muteBtn = nullptr;

    // TTS Studio Tab
    QLineEdit *m_ttsInputEdit = nullptr;
    QComboBox *m_ttsVoiceCombo = nullptr;
    QPushButton *m_ttsGenerateBtn = nullptr;
    QPushButton *m_ttsPlayResultBtn = nullptr;
    QLabel *m_ttsStatusLabel = nullptr;
    QString m_lastTtsAudioPath;

    // Model Manager Tab
    QWidget *m_modelTableWidget = nullptr;
    QLineEdit *m_hfInputEdit = nullptr;
    QLabel *m_modelDownloadStatusLabel = nullptr;
    QProgressBar *m_modelDownloadProgressBar = nullptr;
};
