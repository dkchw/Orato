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
#include <QStackedWidget>

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
    enum class WorkspaceLayout {
        StackedSplit, // Note on top, Transcript under it
        SideSplit,    // Note on left, Transcript on right
        Tabbed        // Classic tabs
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Session management
    void onNewSession();
    void onSaveSession();
    void onDeleteSession();
    void onOpenFolder();
    void onSessionSelected(int index);
    void updateSessionList();

    // Sidebar & Layout controls
    void onSidebarToggle();
    void setWorkspaceLayout(WorkspaceLayout layout);

    // Audio recording & multi-paragraph/retake
    void onRecordToggle();
    void onRecordAppendToggle();
    void onRetake();
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

    // Model Manager UI & Config
    void onDownloadPresetRequested(const QString &presetId);
    void onAddCustomModelClicked();
    void onConvertHfModelClicked();
    void onOpenModelManager();
    void onOpenConfigDialog();
    void onOpenTtsStudio();
    void onToggleSidebarDockSide();

    // Takes management
    void onTakeSelected(int index);
    void onNewTakeClicked();
    void onDeleteTakeClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

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
    void updateTakesUi();

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
    WorkspaceLayout m_currentLayout = WorkspaceLayout::StackedSplit;

    // Splitters & Docking
    QSplitter *m_mainSplitter = nullptr;
    QWidget *m_sidebarWidget = nullptr;
    QWidget *m_studioWidget = nullptr;
    QSplitter *m_workspaceSplitter = nullptr;
    QStackedWidget *m_workspaceStack = nullptr;

    bool m_sidebarOnRight = false;
    int m_savedSidebarWidth = 280;

    // UI Widgets
    QPushButton *m_sidebarToggleBtn = nullptr;
    QPushButton *m_dockSideBtn = nullptr;
    QPushButton *m_openFolderBtn = nullptr;

    QPushButton *m_manageModelsBtn = nullptr;
    QPushButton *m_addModelBtn = nullptr;
    QPushButton *m_configBtn = nullptr;
    QPushButton *m_ttsStudioBtn = nullptr;

    // Top Bar Session Title & Info
    QWidget *m_topBarWidget = nullptr;
    QWidget *m_flowContainer = nullptr;
    QLineEdit *m_topSessionTitleEdit = nullptr;
    QLabel *m_sessionDateBadge = nullptr;
    QLabel *m_sessionTakeBadge = nullptr;

    QComboBox *m_inputDeviceCombo = nullptr;
    QPushButton *m_recordBtn = nullptr;
    QPushButton *m_recordAppendBtn = nullptr;
    QPushButton *m_retakeBtn = nullptr;
    QPushButton *m_pauseBtn = nullptr;
    QLabel *m_recordingTimeLabel = nullptr;
    AudioLevelMeter *m_levelMeter = nullptr;
    QSlider *m_micGainSlider = nullptr;
    QLabel *m_gainValueLabel = nullptr;

    QComboBox *m_modelCombo = nullptr;
    QComboBox *m_languageCombo = nullptr;
    QPushButton *m_transcribeBtn = nullptr;
    QProgressBar *m_transcribeProgress = nullptr;

    // Layout toggles
    QPushButton *m_layoutSplitVBtn = nullptr;
    QPushButton *m_layoutSplitHBtn = nullptr;
    QPushButton *m_layoutTabsBtn = nullptr;

    // Takes & Waveform
    QComboBox *m_takeCombo = nullptr;
    QPushButton *m_newTakeBtn = nullptr;
    QPushButton *m_deleteTakeBtn = nullptr;
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
