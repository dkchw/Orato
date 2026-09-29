#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QGroupBox>
#include <QShortcut>
#include <QKeySequence>
#include <QTableWidget>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Recorder — Speech & Pronunciation Studio"));
    resize(1320, 880);

    // Modern clean dark theme with high contrast and legible typography
    setStyleSheet(
        "QMainWindow { background-color: #0f1013; color: #f8fafc; }"
        "QWidget { color: #f8fafc; font-family: -apple-system, BlinkMacSystemFont, 'Inter', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }"
        "QSplitter::handle { background-color: #272a34; }"
        "QSplitter::handle:horizontal { width: 4px; }"
        "QSplitter::handle:vertical { height: 4px; }"
        "QSplitter::handle:hover { background-color: #4f46e5; }"
        "QGroupBox { font-weight: 600; border: 1px solid #272a34; border-radius: 6px; margin-top: 10px; padding-top: 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #818cf8; }"
        "QPushButton { background-color: #1e2129; border: 1px solid #333846; border-radius: 5px; padding: 6px 12px; color: #f8fafc; font-size: 12px; font-weight: 500; }"
        "QPushButton:hover { background-color: #2a2e3b; border-color: #4f46e5; }"
        "QPushButton:pressed { background-color: #16181f; }"
        "QPushButton:disabled { background-color: #181a20; color: #64748b; border-color: #252833; }"
        "QComboBox { background-color: #1a1c23; border: 1px solid #333846; border-radius: 5px; padding: 4px 10px; color: #f8fafc; min-height: 24px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { background-color: #1a1c23; color: #f8fafc; selection-background-color: #4338ca; border: 1px solid #333846; }"
        "QLineEdit { background-color: #1a1c23; border: 1px solid #333846; border-radius: 5px; padding: 6px 10px; color: #f8fafc; font-size: 12px; }"
        "QLineEdit:focus { border-color: #6366f1; background-color: #20232c; }"
        "QSlider::groove:horizontal { height: 5px; background: #272a34; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #4f46e5; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #818cf8; width: 14px; margin-top: -5px; margin-bottom: -5px; border-radius: 7px; }"
        "QTabWidget::pane { border: 1px solid #272a34; background-color: #14161c; border-radius: 6px; }"
        "QTabBar::tab { background: #1a1c23; color: #94a3b8; padding: 8px 16px; border-top-left-radius: 5px; border-top-right-radius: 5px; margin-right: 3px; font-weight: 600; font-size: 12px; }"
        "QTabBar::tab:selected { background: #14161c; color: #818cf8; border: 1px solid #272a34; border-bottom: none; }"
        "QTabBar::tab:hover:!selected { background: #222631; color: #e2e8f0; }"
        "QListWidget { background-color: #14161c; border: 1px solid #272a34; border-radius: 6px; outline: none; }"
        "QListWidget::item { padding: 9px; border-bottom: 1px solid #1e2129; border-radius: 4px; margin: 1px 2px; }"
        "QListWidget::item:selected { background-color: #262a36; color: #818cf8; }"
        "QListWidget::item:hover:!selected { background-color: #1a1c23; }"
        "QStatusBar { background-color: #14161c; color: #94a3b8; border-top: 1px solid #20232b; font-size: 11px; }"
    );

    // Core managers
    m_recorder = new AudioRecorder(this);
    m_player = new AudioPlayer(this);
    m_whisper = new WhisperEngine(this);
    m_modelManager = new ModelManager(this);
    m_tts = new PocketTTSEngine(this);
    m_sessionManager = new SessionManager(this);

    setupUi();

    // Setup Keyboard Shortcuts
    auto *spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(spaceShortcut, &QShortcut::activated, this, &MainWindow::onPlayToggle);

    auto *replayShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    connect(replayShortcut, &QShortcut::activated, this, &MainWindow::onReplayCurrentSentence);

    auto *sidebarShortcut = new QShortcut(QKeySequence("Ctrl+B"), this);
    connect(sidebarShortcut, &QShortcut::activated, this, &MainWindow::onSidebarToggle);

    // Initial hardware and models discovery
    refreshAudioDevices();
    refreshModelList();
    updateSessionList();

    // Load initial session
    auto sessions = m_sessionManager->listSessions();
    if (!sessions.isEmpty()) {
        m_sessionListWidget->setCurrentRow(0);
    } else {
        onNewSession();
    }

    statusBar()->showMessage(tr("Ready. Choose audio input source and record, or review notes and sentences."));
}

void MainWindow::setupUi() {
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(0);

    // Resizable Main Splitter (Left Sidebar + Right Studio)
    m_mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    m_mainSplitter->setChildrenCollapsible(true);

    // 1. Left Sidebar
    m_sidebarWidget = new QWidget(m_mainSplitter);
    setupSidebar(m_sidebarWidget);
    m_mainSplitter->addWidget(m_sidebarWidget);

    // 2. Right Studio Area
    auto *studioWidget = new QWidget(m_mainSplitter);
    auto *studioLayout = new QVBoxLayout(studioWidget);
    studioLayout->setContentsMargins(6, 4, 4, 4);
    studioLayout->setSpacing(6);

    // Top Bar (Recording + STT controls)
    auto *topBarWidget = new QWidget(studioWidget);
    setupTopBar(topBarWidget);
    studioLayout->addWidget(topBarWidget);

    // Waveform Timeline Area
    auto *waveformContainer = new QWidget(studioWidget);
    setupWaveformArea(waveformContainer);
    studioLayout->addWidget(waveformContainer);

    // Workspace Area: Stacked widget containing Splitter mode (Note + Transcript) or Tabs mode
    m_workspaceStack = new QStackedWidget(studioWidget);

    // Sub-components: Note Editor & Sentence List
    m_noteEditor = new MarkdownNoteEditor(this);
    connect(m_noteEditor, &MarkdownNoteEditor::textChanged, this, [this]() {
        m_currentSession.noteMarkdown = m_noteEditor->markdownText();
    });
    connect(m_noteEditor, &MarkdownNoteEditor::timestampClicked,
            this, &MainWindow::onNoteTimestampClicked);

    m_sentenceListView = new SentenceListView(this);
    connect(m_sentenceListView, &SentenceListView::playSentenceRequested,
            this, [this](qint64 startMs, qint64 endMs, bool loop) {
        m_player->playSegment(startMs, endMs, loop);
    });
    connect(m_sentenceListView, &SentenceListView::ttsSentenceRequested,
            this, &MainWindow::onTtsSentenceRequested);
    connect(m_sentenceListView, &SentenceListView::appendToNoteRequested,
            this, [this](qint64 ms, const QString &text) {
        m_noteEditor->appendSentence(ms, text);
    });
    connect(m_sentenceListView, &SentenceListView::segmentsChanged, this, [this]() {
        m_currentSession.segments = m_sentenceListView->segments();
        m_waveformWidget->setSegments(m_currentSession.segments);
    });

    // Page 0: Resizable Workspace Splitter (for Stacked Note-over-Transcript or Side-by-Side)
    m_workspaceSplitter = new QSplitter(Qt::Vertical, m_workspaceStack);
    m_workspaceSplitter->addWidget(m_noteEditor);
    m_workspaceSplitter->addWidget(m_sentenceListView);
    m_workspaceSplitter->setStretchFactor(0, 1);
    m_workspaceSplitter->setStretchFactor(1, 1);
    m_workspaceStack->addWidget(m_workspaceSplitter);

    // Page 1: Tabbed Widget (Sentences, Notes, TTS Studio, Models)
    m_tabWidget = new QTabWidget(m_workspaceStack);

    auto *ttsTabWidget = new QWidget(this);
    setupTtsStudioTab(ttsTabWidget);
    m_tabWidget->addTab(ttsTabWidget, QIcon(":/icons/volume.svg"), tr("TTS Studio"));

    auto *modelTabWidget = new QWidget(this);
    setupModelManagerTab(modelTabWidget);
    m_tabWidget->addTab(modelTabWidget, QIcon(":/icons/cpu.svg"), tr("Whisper Models"));

    m_workspaceStack->addWidget(m_tabWidget);

    studioLayout->addWidget(m_workspaceStack, 1);

    // Bottom Bar (Full Playback Controls)
    auto *bottomBarWidget = new QWidget(studioWidget);
    setupBottomBar(bottomBarWidget);
    studioLayout->addWidget(bottomBarWidget);

    m_mainSplitter->addWidget(studioWidget);

    // Default splitter proportions (Sidebar: 260px, Studio: 1040px)
    m_mainSplitter->setSizes({260, 1060});

    mainLayout->addWidget(m_mainSplitter);

    // Connect core recorder signals
    connect(m_recorder, &AudioRecorder::durationChanged, this, &MainWindow::onRecordingDurationChanged);
    connect(m_recorder, &AudioRecorder::levelChanged, m_levelMeter, &AudioLevelMeter::setLevels);
    connect(m_recorder, &AudioRecorder::recordingFinished, this, &MainWindow::onRecordingFinished);

    // Connect core player signals
    connect(m_player, &AudioPlayer::positionChanged, this, &MainWindow::onPlaybackPositionChanged);
    connect(m_player, &AudioPlayer::durationChanged, this, &MainWindow::onPlaybackDurationChanged);
    connect(m_player, &AudioPlayer::playbackStateChanged, this, &MainWindow::onPlaybackStateChanged);

    // Connect Whisper signals
    connect(m_whisper, &WhisperEngine::transcriptionStarted, this, &MainWindow::onWhisperStarted);
    connect(m_whisper, &WhisperEngine::progress, this, &MainWindow::onWhisperProgress);
    connect(m_whisper, &WhisperEngine::segmentDiscovered, this, &MainWindow::onWhisperSegmentDiscovered);
    connect(m_whisper, &WhisperEngine::transcriptionCompleted, this, &MainWindow::onWhisperCompleted);
    connect(m_whisper, &WhisperEngine::transcriptionFailed, this, &MainWindow::onWhisperFailed);

    // Connect TTS engine signals
    connect(m_tts, &PocketTTSEngine::generationStarted, this, &MainWindow::onTtsStarted);
    connect(m_tts, &PocketTTSEngine::generationCompleted, this, &MainWindow::onTtsCompleted);
    connect(m_tts, &PocketTTSEngine::generationFailed, this, &MainWindow::onTtsFailed);

    // Connect ModelManager signals
    connect(m_modelManager, &ModelManager::modelsListChanged, this, &MainWindow::refreshModelList);

    setWorkspaceLayout(WorkspaceLayout::StackedSplit);
}

void MainWindow::setupSidebar(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    auto *headerRow = new QHBoxLayout();
    auto *titleLabel = new QLabel(tr("Sessions"), container);
    titleLabel->setStyleSheet("font-weight: 700; font-size: 13px; color: #818cf8; letter-spacing: 0.5px;");
    headerRow->addWidget(titleLabel);
    headerRow->addStretch();

    // Open Folder Button
    m_openFolderBtn = new QPushButton(container);
    m_openFolderBtn->setIcon(QIcon(":/icons/folder-open.svg"));
    m_openFolderBtn->setToolTip(tr("Open Session Folder in File Manager"));
    m_openFolderBtn->setFixedSize(28, 28);
    connect(m_openFolderBtn, &QPushButton::clicked, this, &MainWindow::onOpenFolder);
    headerRow->addWidget(m_openFolderBtn);

    layout->addLayout(headerRow);

    // Action buttons row: New, Save, Delete
    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(4);

    auto *newBtn = new QPushButton(tr("New"), container);
    newBtn->setIcon(QIcon(":/icons/plus.svg"));
    newBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600;");
    connect(newBtn, &QPushButton::clicked, this, &MainWindow::onNewSession);
    btnRow->addWidget(newBtn);

    auto *saveBtn = new QPushButton(tr("Save"), container);
    saveBtn->setIcon(QIcon(":/icons/save.svg"));
    saveBtn->setStyleSheet("background-color: #4338ca; color: white; font-weight: 600;");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveSession);
    btnRow->addWidget(saveBtn);

    auto *delBtn = new QPushButton(container);
    delBtn->setIcon(QIcon(":/icons/trash-2.svg"));
    delBtn->setToolTip(tr("Delete Session"));
    delBtn->setStyleSheet("background-color: #991b1b; color: white;");
    delBtn->setFixedSize(28, 28);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::onDeleteSession);
    btnRow->addWidget(delBtn);

    layout->addLayout(btnRow);

    // Sessions List
    m_sessionListWidget = new QListWidget(container);
    connect(m_sessionListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onSessionSelected);
    layout->addWidget(m_sessionListWidget, 1);

    m_sessionTitleEdit = new QLineEdit(container);
    m_sessionTitleEdit->setPlaceholderText(tr("Session Title"));
    connect(m_sessionTitleEdit, &QLineEdit::textEdited, this, [this](const QString &t) {
        m_currentSession.title = t;
    });
    layout->addWidget(m_sessionTitleEdit);
}

void MainWindow::setupTopBar(QWidget *container) {
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(6);

    // Sidebar toggle button
    m_sidebarToggleBtn = new QPushButton(container);
    m_sidebarToggleBtn->setIcon(QIcon(":/icons/sidebar.svg"));
    m_sidebarToggleBtn->setToolTip(tr("Toggle Left Sidebar [Ctrl+B]"));
    m_sidebarToggleBtn->setFixedSize(30, 30);
    connect(m_sidebarToggleBtn, &QPushButton::clicked, this, &MainWindow::onSidebarToggle);
    topRow->addWidget(m_sidebarToggleBtn);

    // Audio input selector
    topRow->addWidget(new QLabel(tr("Input:"), container));
    m_inputDeviceCombo = new QComboBox(container);
    m_inputDeviceCombo->setMinimumWidth(160);
    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAudioInputDeviceChanged);
    topRow->addWidget(m_inputDeviceCombo);

    auto *refreshDevBtn = new QPushButton(container);
    refreshDevBtn->setIcon(QIcon(":/icons/refresh-cw.svg"));
    refreshDevBtn->setToolTip(tr("Refresh Audio Devices"));
    refreshDevBtn->setFixedSize(28, 28);
    connect(refreshDevBtn, &QPushButton::clicked, this, &MainWindow::refreshAudioDevices);
    topRow->addWidget(refreshDevBtn);

    // Record Button (New Take)
    m_recordBtn = new QPushButton(tr("Record"), container);
    m_recordBtn->setIcon(QIcon(":/icons/record.svg"));
    m_recordBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: 600; padding: 6px 14px;");
    m_recordBtn->setToolTip(tr("Record a fresh take (replaces previous audio)"));
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::onRecordToggle);
    topRow->addWidget(m_recordBtn);

    // Append Record Button (Record More / Multi-sentence)
    m_recordAppendBtn = new QPushButton(tr("Append"), container);
    m_recordAppendBtn->setIcon(QIcon(":/icons/mic-plus.svg"));
    m_recordAppendBtn->setStyleSheet("background-color: #b91c1c; color: white; font-weight: 500;");
    m_recordAppendBtn->setToolTip(tr("Record additional sentences/paragraphs to existing session audio"));
    connect(m_recordAppendBtn, &QPushButton::clicked, this, &MainWindow::onRecordAppendToggle);
    topRow->addWidget(m_recordAppendBtn);

    // Retake Button
    m_retakeBtn = new QPushButton(tr("Retake"), container);
    m_retakeBtn->setIcon(QIcon(":/icons/rotate-ccw.svg"));
    m_retakeBtn->setToolTip(tr("Clear existing recording and prepare for a fresh take"));
    connect(m_retakeBtn, &QPushButton::clicked, this, &MainWindow::onRetake);
    topRow->addWidget(m_retakeBtn);

    m_pauseBtn = new QPushButton(container);
    m_pauseBtn->setIcon(QIcon(":/icons/pause.svg"));
    m_pauseBtn->setToolTip(tr("Pause / Resume recording"));
    m_pauseBtn->setEnabled(false);
    m_pauseBtn->setFixedSize(28, 28);
    connect(m_pauseBtn, &QPushButton::clicked, this, &MainWindow::onPauseToggle);
    topRow->addWidget(m_pauseBtn);

    m_recordingTimeLabel = new QLabel("00:00", container);
    m_recordingTimeLabel->setStyleSheet("font-family: monospace; font-size: 13px; font-weight: bold; color: #f87171;");
    topRow->addWidget(m_recordingTimeLabel);

    // Live VU Meter
    m_levelMeter = new AudioLevelMeter(container);
    m_levelMeter->setFixedWidth(70);
    topRow->addWidget(m_levelMeter);

    topRow->addSpacing(10);

    // Whisper Model selector
    topRow->addWidget(new QLabel(tr("Model:"), container));
    m_modelCombo = new QComboBox(container);
    m_modelCombo->setMinimumWidth(180);
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModelSelectionChanged);
    topRow->addWidget(m_modelCombo);

    topRow->addWidget(new QLabel(tr("Lang:"), container));
    m_languageCombo = new QComboBox(container);
    m_languageCombo->addItem("German (de)", "de");
    m_languageCombo->addItem("English (en)", "en");
    m_languageCombo->addItem("Auto Detect", "auto");
    m_languageCombo->addItem("French (fr)", "fr");
    m_languageCombo->addItem("Spanish (es)", "es");
    m_languageCombo->addItem("Italian (it)", "it");
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        QString lang = m_languageCombo->itemData(idx).toString();
        m_currentSession.language = lang;
        m_whisper->setLanguage(lang);
    });
    topRow->addWidget(m_languageCombo);

    // Transcribe Button
    m_transcribeBtn = new QPushButton(tr("Transcribe"), container);
    m_transcribeBtn->setIcon(QIcon(":/icons/zap.svg"));
    m_transcribeBtn->setStyleSheet("background-color: #4f46e5; color: white; font-weight: 600; padding: 6px 14px;");
    connect(m_transcribeBtn, &QPushButton::clicked, this, &MainWindow::onTranscribeClicked);
    topRow->addWidget(m_transcribeBtn);

    topRow->addSpacing(8);

    // Layout switcher buttons
    topRow->addWidget(new QLabel(tr("Layout:"), container));
    m_layoutSplitVBtn = new QPushButton(container);
    m_layoutSplitVBtn->setIcon(QIcon(":/icons/layout-split-v.svg"));
    m_layoutSplitVBtn->setToolTip(tr("Stacked View (Notes on top, Transcript under it)"));
    m_layoutSplitVBtn->setFixedSize(28, 28);
    connect(m_layoutSplitVBtn, &QPushButton::clicked, this, [this]() {
        setWorkspaceLayout(WorkspaceLayout::StackedSplit);
    });
    topRow->addWidget(m_layoutSplitVBtn);

    m_layoutSplitHBtn = new QPushButton(container);
    m_layoutSplitHBtn->setIcon(QIcon(":/icons/layout-split-h.svg"));
    m_layoutSplitHBtn->setToolTip(tr("Side-by-Side View (Notes left, Transcript right)"));
    m_layoutSplitHBtn->setFixedSize(28, 28);
    connect(m_layoutSplitHBtn, &QPushButton::clicked, this, [this]() {
        setWorkspaceLayout(WorkspaceLayout::SideSplit);
    });
    topRow->addWidget(m_layoutSplitHBtn);

    m_layoutTabsBtn = new QPushButton(container);
    m_layoutTabsBtn->setIcon(QIcon(":/icons/layout-tabs.svg"));
    m_layoutTabsBtn->setToolTip(tr("Tabs View (Sentences, Notes, TTS Studio, Models)"));
    m_layoutTabsBtn->setFixedSize(28, 28);
    connect(m_layoutTabsBtn, &QPushButton::clicked, this, [this]() {
        setWorkspaceLayout(WorkspaceLayout::Tabbed);
    });
    topRow->addWidget(m_layoutTabsBtn);

    topRow->addStretch();
    mainLayout->addLayout(topRow);

    // Transcribe Progress Bar
    m_transcribeProgress = new QProgressBar(container);
    m_transcribeProgress->setRange(0, 100);
    m_transcribeProgress->setValue(0);
    m_transcribeProgress->setFixedHeight(5);
    m_transcribeProgress->setTextVisible(false);
    m_transcribeProgress->setStyleSheet(
        "QProgressBar { background: #1a1c23; border: none; border-radius: 2px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #818cf8, stop:1 #34d399); border-radius: 2px; }"
    );
    m_transcribeProgress->hide();
    mainLayout->addWidget(m_transcribeProgress);
}

void MainWindow::setupWaveformArea(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto *topRow = new QHBoxLayout();
    topRow->setContentsMargins(4, 0, 4, 0);

    auto *timelineTitle = new QLabel(tr("Timeline & Waveform"), container);
    timelineTitle->setStyleSheet("font-size: 11px; font-weight: 600; color: #818cf8;");
    topRow->addWidget(timelineTitle);

    topRow->addStretch();

    auto *zoomInBtn = new QPushButton(container);
    zoomInBtn->setIcon(QIcon(":/icons/zoom-in.svg"));
    zoomInBtn->setToolTip(tr("Zoom In [Ctrl + Mouse Wheel]"));
    zoomInBtn->setFixedSize(26, 24);
    connect(zoomInBtn, &QPushButton::clicked, this, [this]() { m_waveformWidget->zoomIn(); });
    topRow->addWidget(zoomInBtn);

    auto *zoomOutBtn = new QPushButton(container);
    zoomOutBtn->setIcon(QIcon(":/icons/zoom-out.svg"));
    zoomOutBtn->setToolTip(tr("Zoom Out"));
    zoomOutBtn->setFixedSize(26, 24);
    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() { m_waveformWidget->zoomOut(); });
    topRow->addWidget(zoomOutBtn);

    auto *zoomFitBtn = new QPushButton(container);
    zoomFitBtn->setIcon(QIcon(":/icons/maximize-2.svg"));
    zoomFitBtn->setToolTip(tr("Fit Waveform to Window"));
    zoomFitBtn->setFixedSize(26, 24);
    connect(zoomFitBtn, &QPushButton::clicked, this, [this]() { m_waveformWidget->zoomFit(); });
    topRow->addWidget(zoomFitBtn);

    layout->addLayout(topRow);

    m_waveformWidget = new WaveformWidget(container);
    connect(m_waveformWidget, &WaveformWidget::seekRequested, this, &MainWindow::onTimelineSeekRequested);
    connect(m_waveformWidget, &WaveformWidget::segmentClicked, this, &MainWindow::onTimelineSegmentClicked);
    connect(m_waveformWidget, &WaveformWidget::segmentDoubleClicked, this, &MainWindow::onTimelineSegmentDoubleClicked);

    layout->addWidget(m_waveformWidget);
}

void MainWindow::setupBottomBar(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    // Scrubber slider row
    auto *sliderRow = new QHBoxLayout();
    m_timelineSlider = new QSlider(Qt::Horizontal, container);
    m_timelineSlider->setRange(0, 1000);
    connect(m_timelineSlider, &QSlider::sliderMoved, this, [this](int val) {
        qint64 dur = m_player->duration();
        if (dur > 0) {
            qint64 targetMs = (val * dur) / 1000;
            m_player->seek(targetMs);
        }
    });
    sliderRow->addWidget(m_timelineSlider, 1);

    m_playbackTimeLabel = new QLabel("00:00.0 / 00:00.0", container);
    m_playbackTimeLabel->setStyleSheet("font-family: monospace; font-size: 12px; color: #94a3b8;");
    sliderRow->addWidget(m_playbackTimeLabel);

    layout->addLayout(sliderRow);

    // Playback buttons
    auto *ctrlRow = new QHBoxLayout();
    ctrlRow->setSpacing(6);

    m_playBtn = new QPushButton(tr("Play"), container);
    m_playBtn->setIcon(QIcon(":/icons/play.svg"));
    m_playBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600; min-width: 80px;");
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayToggle);
    ctrlRow->addWidget(m_playBtn);

    m_stopBtn = new QPushButton(container);
    m_stopBtn->setIcon(QIcon(":/icons/stop.svg"));
    m_stopBtn->setToolTip(tr("Stop playback"));
    m_stopBtn->setFixedSize(32, 30);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    ctrlRow->addWidget(m_stopBtn);

    m_skipBackBtn = new QPushButton(tr("-5s"), container);
    m_skipBackBtn->setIcon(QIcon(":/icons/skip-back.svg"));
    connect(m_skipBackBtn, &QPushButton::clicked, this, [this]() { m_player->skipBackward(5000); });
    ctrlRow->addWidget(m_skipBackBtn);

    m_skipForwardBtn = new QPushButton(tr("+5s"), container);
    m_skipForwardBtn->setIcon(QIcon(":/icons/skip-forward.svg"));
    connect(m_skipForwardBtn, &QPushButton::clicked, this, [this]() { m_player->skipForward(5000); });
    ctrlRow->addWidget(m_skipForwardBtn);

    ctrlRow->addSpacing(10);

    m_replaySentenceBtn = new QPushButton(tr("Replay Sentence [R]"), container);
    m_replaySentenceBtn->setIcon(QIcon(":/icons/replay.svg"));
    m_replaySentenceBtn->setStyleSheet("background-color: #4338ca; color: white; font-weight: 600;");
    connect(m_replaySentenceBtn, &QPushButton::clicked, this, &MainWindow::onReplayCurrentSentence);
    ctrlRow->addWidget(m_replaySentenceBtn);

    m_loopSentenceBtn = new QPushButton(tr("Loop Sentence"), container);
    m_loopSentenceBtn->setIcon(QIcon(":/icons/loop.svg"));
    m_loopSentenceBtn->setCheckable(true);
    connect(m_loopSentenceBtn, &QPushButton::toggled, this, &MainWindow::onLoopSentenceToggle);
    ctrlRow->addWidget(m_loopSentenceBtn);

    ctrlRow->addStretch();

    // Speed Selector
    ctrlRow->addWidget(new QLabel(tr("Speed:"), container));
    m_speedCombo = new QComboBox(container);
    m_speedCombo->addItem("0.5x", 0.5);
    m_speedCombo->addItem("0.75x", 0.75);
    m_speedCombo->addItem("1.0x", 1.0);
    m_speedCombo->addItem("1.25x", 1.25);
    m_speedCombo->addItem("1.5x", 1.5);
    m_speedCombo->addItem("2.0x", 2.0);
    m_speedCombo->setCurrentIndex(2); // 1.0x
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSpeedChanged);
    ctrlRow->addWidget(m_speedCombo);

    ctrlRow->addSpacing(10);

    // Volume & Mute
    m_muteBtn = new QPushButton(container);
    m_muteBtn->setIcon(QIcon(":/icons/volume.svg"));
    m_muteBtn->setFixedSize(30, 30);
    connect(m_muteBtn, &QPushButton::clicked, this, &MainWindow::onMuteToggle);
    ctrlRow->addWidget(m_muteBtn);

    m_volumeSlider = new QSlider(Qt::Horizontal, container);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(80);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderChanged);
    ctrlRow->addWidget(m_volumeSlider);

    layout->addLayout(ctrlRow);
}

void MainWindow::setupTtsStudioTab(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *descLabel = new QLabel(
        tr("<b>Kyutai Pocket TTS Studio:</b> High-speed CPU text-to-speech for language practice and native accent comparison."),
        container
    );
    descLabel->setStyleSheet("color: #818cf8; font-size: 13px;");
    layout->addWidget(descLabel);

    auto *inputGroup = new QGroupBox(tr("Speech Synthesis"), container);
    auto *groupLayout = new QVBoxLayout(inputGroup);

    m_ttsInputEdit = new QLineEdit(inputGroup);
    m_ttsInputEdit->setPlaceholderText(tr("Type text to synthesize with Pocket TTS..."));
    m_ttsInputEdit->setText("Guten Tag! Ich lerne heute Deutsch mit dem Recorder Studio.");
    groupLayout->addWidget(m_ttsInputEdit);

    auto *optRow = new QHBoxLayout();
    optRow->addWidget(new QLabel(tr("Voice:"), inputGroup));
    m_ttsVoiceCombo = new QComboBox(inputGroup);
    for (const auto &v : m_tts->availableVoices()) {
        m_ttsVoiceCombo->addItem(v.displayName, v.id);
    }
    optRow->addWidget(m_ttsVoiceCombo);

    m_ttsGenerateBtn = new QPushButton(tr("Generate Speech"), inputGroup);
    m_ttsGenerateBtn->setIcon(QIcon(":/icons/volume.svg"));
    m_ttsGenerateBtn->setStyleSheet("background-color: #d97706; color: white; font-weight: 600;");
    connect(m_ttsGenerateBtn, &QPushButton::clicked, this, &MainWindow::onTtsStudioGenerate);
    optRow->addWidget(m_ttsGenerateBtn);

    m_ttsPlayResultBtn = new QPushButton(tr("Play Generated"), inputGroup);
    m_ttsPlayResultBtn->setIcon(QIcon(":/icons/play.svg"));
    m_ttsPlayResultBtn->setEnabled(false);
    connect(m_ttsPlayResultBtn, &QPushButton::clicked, this, [this]() {
        if (!m_lastTtsAudioPath.isEmpty()) {
            m_player->setSource(m_lastTtsAudioPath);
            m_player->play();
        }
    });
    optRow->addWidget(m_ttsPlayResultBtn);

    optRow->addStretch();
    groupLayout->addLayout(optRow);

    m_ttsStatusLabel = new QLabel(tr("Pocket TTS ready for CPU inference."), inputGroup);
    m_ttsStatusLabel->setStyleSheet("color: #94a3b8; font-size: 11px;");
    groupLayout->addWidget(m_ttsStatusLabel);

    layout->addWidget(inputGroup);
    layout->addStretch();
}

void MainWindow::setupModelManagerTab(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *hdr = new QLabel(
        tr("<b>Whisper.cpp Models Manager:</b> Download fine-tuned German model or official Whisper models, or convert HuggingFace models."),
        container
    );
    hdr->setStyleSheet("color: #818cf8; font-size: 13px;");
    layout->addWidget(hdr);

    auto *table = new QTableWidget(container);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({tr("Model Name"), tr("Language"), tr("Size"), tr("Action")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->setStyleSheet("QTableWidget { background-color: #1a1c23; border: 1px solid #272a34; }");

    auto presets = m_modelManager->presetModels();
    table->setRowCount(presets.size());

    for (int r = 0; r < presets.size(); ++r) {
        const auto &p = presets[r];
        table->setItem(r, 0, new QTableWidgetItem(p.name));
        table->setItem(r, 1, new QTableWidgetItem(p.language.toUpper()));
        table->setItem(r, 2, new QTableWidgetItem(QString("%1 MB").arg(p.approxSizeMb)));

        bool installed = m_modelManager->isModelInstalled(p.id);
        auto *actionBtn = new QPushButton(installed ? tr("Installed") : tr("Download"), table);
        if (installed) {
            actionBtn->setEnabled(false);
            actionBtn->setStyleSheet("background-color: #065f46; color: #6ee7b7; font-weight: 600;");
        } else {
            actionBtn->setStyleSheet("background-color: #4338ca; color: white;");
            QString pid = p.id;
            connect(actionBtn, &QPushButton::clicked, this, [this, pid]() {
                onDownloadPresetRequested(pid);
            });
        }
        table->setCellWidget(r, 3, actionBtn);
    }
    layout->addWidget(table, 1);

    m_modelDownloadStatusLabel = new QLabel(container);
    m_modelDownloadStatusLabel->setStyleSheet("color: #818cf8; font-size: 11px;");
    layout->addWidget(m_modelDownloadStatusLabel);

    m_modelDownloadProgressBar = new QProgressBar(container);
    m_modelDownloadProgressBar->setRange(0, 100);
    m_modelDownloadProgressBar->setValue(0);
    m_modelDownloadProgressBar->hide();
    layout->addWidget(m_modelDownloadProgressBar);

    connect(m_modelManager, &ModelManager::downloadStarted, this, [this](const QString &name) {
        m_modelDownloadStatusLabel->setText(tr("Downloading %1...").arg(name));
        m_modelDownloadProgressBar->setValue(0);
        m_modelDownloadProgressBar->show();
    });
    connect(m_modelManager, &ModelManager::downloadProgress, this, [this](qint64, qint64, int pct) {
        m_modelDownloadProgressBar->setValue(pct);
    });
    connect(m_modelManager, &ModelManager::downloadCompleted, this, [this](const QString &path) {
        m_modelDownloadStatusLabel->setText(tr("Successfully installed: %1").arg(path));
        m_modelDownloadProgressBar->hide();
        refreshModelList();
    });
    connect(m_modelManager, &ModelManager::downloadFailed, this, [this](const QString &err) {
        m_modelDownloadStatusLabel->setText(tr("Download failed: %1").arg(err));
        m_modelDownloadProgressBar->hide();
    });

    auto *bottomRow = new QHBoxLayout();
    auto *addCustomBtn = new QPushButton(tr("Browse Custom Model..."), container);
    addCustomBtn->setIcon(QIcon(":/icons/folder.svg"));
    connect(addCustomBtn, &QPushButton::clicked, this, &MainWindow::onAddCustomModelClicked);
    bottomRow->addWidget(addCustomBtn);

    bottomRow->addSpacing(16);

    bottomRow->addWidget(new QLabel(tr("Hugging Face Model:"), container));
    m_hfInputEdit = new QLineEdit("https://huggingface.co/primeline/whisper-tiny-german", container);
    bottomRow->addWidget(m_hfInputEdit, 1);

    auto *convertBtn = new QPushButton(tr("Convert to GGML"), container);
    convertBtn->setIcon(QIcon(":/icons/zap.svg"));
    connect(convertBtn, &QPushButton::clicked, this, &MainWindow::onConvertHfModelClicked);
    bottomRow->addWidget(convertBtn);

    layout->addLayout(bottomRow);
}

// -----------------------------------------------------------------------------
// Layout & Sidebar Slots
// -----------------------------------------------------------------------------

void MainWindow::onSidebarToggle() {
    m_sidebarWidget->setVisible(!m_sidebarWidget->isVisible());
}

void MainWindow::setWorkspaceLayout(WorkspaceLayout layout) {
    m_currentLayout = layout;

    m_layoutSplitVBtn->setStyleSheet(layout == WorkspaceLayout::StackedSplit
        ? "background-color: #4f46e5; border-color: #818cf8;" : "");
    m_layoutSplitHBtn->setStyleSheet(layout == WorkspaceLayout::SideSplit
        ? "background-color: #4f46e5; border-color: #818cf8;" : "");
    m_layoutTabsBtn->setStyleSheet(layout == WorkspaceLayout::Tabbed
        ? "background-color: #4f46e5; border-color: #818cf8;" : "");

    if (layout == WorkspaceLayout::StackedSplit) {
        m_workspaceStack->setCurrentIndex(0);
        m_workspaceSplitter->setOrientation(Qt::Vertical);
        m_workspaceSplitter->addWidget(m_noteEditor);
        m_workspaceSplitter->addWidget(m_sentenceListView);
        m_noteEditor->show();
        m_sentenceListView->show();
        m_workspaceSplitter->setSizes({400, 350});
    } else if (layout == WorkspaceLayout::SideSplit) {
        m_workspaceStack->setCurrentIndex(0);
        m_workspaceSplitter->setOrientation(Qt::Horizontal);
        m_workspaceSplitter->addWidget(m_noteEditor);
        m_workspaceSplitter->addWidget(m_sentenceListView);
        m_noteEditor->show();
        m_sentenceListView->show();
        m_workspaceSplitter->setSizes({500, 500});
    } else if (layout == WorkspaceLayout::Tabbed) {
        // Tabbed mode
        m_tabWidget->insertTab(0, m_noteEditor, QIcon(":/icons/file-text.svg"), tr("Notes"));
        m_tabWidget->insertTab(1, m_sentenceListView, QIcon(":/icons/subtitles.svg"), tr("Sentences"));
        m_tabWidget->setCurrentIndex(0);
        m_workspaceStack->setCurrentIndex(1);
    }
}

void MainWindow::onOpenFolder() {
    QString sessionDir = m_sessionManager->sessionDirectory(m_currentSession.id);
    QDesktopServices::openUrl(QUrl::fromLocalFile(sessionDir));
}

// -----------------------------------------------------------------------------
// Audio Recording Slots
// -----------------------------------------------------------------------------

void MainWindow::refreshAudioDevices() {
    m_inputDeviceCombo->clear();
    auto devices = m_recorder->availableDevices();
    auto defaultDev = m_recorder->defaultDevice();

    int defaultIndex = 0;
    for (int i = 0; i < devices.size(); ++i) {
        m_inputDeviceCombo->addItem(devices[i].description(), QVariant::fromValue(devices[i]));
        if (devices[i].id() == defaultDev.id()) {
            defaultIndex = i;
        }
    }
    if (!devices.isEmpty()) {
        m_inputDeviceCombo->setCurrentIndex(defaultIndex);
    }
}

void MainWindow::onAudioInputDeviceChanged(int) {
    // Current device will be used on next record
}

void MainWindow::onRecordToggle() {
    if (m_recorder->state() == AudioRecorder::State::Recording) {
        QString audioPath = m_sessionManager->getAudioPath(m_currentSession.id);
        m_recorder->stopRecording(audioPath);
        m_recordBtn->setText(tr("Record"));
        m_recordBtn->setIcon(QIcon(":/icons/record.svg"));
        m_recordBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: 600; padding: 6px 14px;");
        m_recordAppendBtn->setEnabled(true);
        m_pauseBtn->setEnabled(false);
    } else {
        QVariant data = m_inputDeviceCombo->currentData();
        QAudioDevice dev = data.value<QAudioDevice>();

        if (m_recorder->startRecording(dev, false)) {
            m_recordBtn->setText(tr("Stop"));
            m_recordBtn->setIcon(QIcon(":/icons/stop.svg"));
            m_recordBtn->setStyleSheet("background-color: #b91c1c; color: #fef08a; font-weight: 700; border: 2px solid #eab308;");
            m_recordAppendBtn->setEnabled(false);
            m_pauseBtn->setEnabled(true);
            m_pauseBtn->setIcon(QIcon(":/icons/pause.svg"));
            statusBar()->showMessage(tr("Recording fresh take..."));
        }
    }
}

void MainWindow::onRecordAppendToggle() {
    if (m_recorder->state() == AudioRecorder::State::Recording) {
        QString audioPath = m_sessionManager->getAudioPath(m_currentSession.id);
        m_recorder->stopRecording(audioPath);
        m_recordAppendBtn->setText(tr("Append"));
        m_recordAppendBtn->setIcon(QIcon(":/icons/mic-plus.svg"));
        m_recordAppendBtn->setStyleSheet("background-color: #b91c1c; color: white; font-weight: 500;");
        m_recordBtn->setEnabled(true);
        m_pauseBtn->setEnabled(false);
    } else {
        QVariant data = m_inputDeviceCombo->currentData();
        QAudioDevice dev = data.value<QAudioDevice>();

        // Pass existing samples to recorder before appending
        m_recorder->setPcmSamples(m_currentAudioPcm);

        if (m_recorder->startRecording(dev, true)) {
            m_recordAppendBtn->setText(tr("Stop"));
            m_recordAppendBtn->setIcon(QIcon(":/icons/stop.svg"));
            m_recordAppendBtn->setStyleSheet("background-color: #7f1d1d; color: #fef08a; font-weight: 700; border: 2px solid #f97316;");
            m_recordBtn->setEnabled(false);
            m_pauseBtn->setEnabled(true);
            statusBar()->showMessage(tr("Appending to session audio (multi-paragraph mode)..."));
        }
    }
}

void MainWindow::onRetake() {
    auto reply = QMessageBox::question(
        this, tr("Retake Audio"),
        tr("Are you sure you want to clear current audio and start a new take?"),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        if (m_recorder->state() == AudioRecorder::State::Recording) {
            m_recorder->stopRecording();
        }
        m_recorder->clearAudio();
        m_currentAudioPcm.clear();
        m_currentSession.durationMs = 0;
        m_waveformWidget->setAudioData(m_currentAudioPcm, 0);
        m_player->stop();
        m_player->setSource(QString());
        m_recordingTimeLabel->setText("00:00");
        statusBar()->showMessage(tr("Audio cleared. Ready for fresh take."));
    }
}

void MainWindow::onPauseToggle() {
    if (m_recorder->state() == AudioRecorder::State::Recording) {
        m_recorder->pauseRecording();
        m_pauseBtn->setIcon(QIcon(":/icons/play.svg"));
        statusBar()->showMessage(tr("Recording paused."));
    } else if (m_recorder->state() == AudioRecorder::State::Paused) {
        m_recorder->resumeRecording();
        m_pauseBtn->setIcon(QIcon(":/icons/pause.svg"));
        statusBar()->showMessage(tr("Recording resumed..."));
    }
}

void MainWindow::onRecordingDurationChanged(qint64 ms) {
    m_recordingTimeLabel->setText(formatTime(ms));
}

void MainWindow::onRecordingFinished(const QString &savedPath, qint64 durationMs) {
    m_currentAudioPcm = m_recorder->pcmSamples();
    m_currentSession.durationMs = durationMs;
    m_currentSession.audioFileName = "audio.wav";

    m_waveformWidget->setAudioData(m_currentAudioPcm, durationMs);
    m_player->setSource(savedPath);

    onSaveSession();
    statusBar()->showMessage(tr("Recording complete. Duration: %1. Ready to transcribe.").arg(formatTime(durationMs)));
}

// -----------------------------------------------------------------------------
// Playback Slots
// -----------------------------------------------------------------------------

void MainWindow::onPlayToggle() {
    m_player->togglePlayPause();
}

void MainWindow::onStop() {
    m_player->stop();
}

void MainWindow::onPlaybackPositionChanged(qint64 ms) {
    m_waveformWidget->setPlaybackPosition(ms);
    m_sentenceListView->updatePlaybackPosition(ms);

    qint64 dur = m_player->duration();
    if (dur > 0) {
        int sliderVal = static_cast<int>((ms * 1000) / dur);
        m_timelineSlider->blockSignals(true);
        m_timelineSlider->setValue(sliderVal);
        m_timelineSlider->blockSignals(false);
    }

    m_playbackTimeLabel->setText(QString("%1 / %2").arg(formatTime(ms), formatTime(dur)));
}

void MainWindow::onPlaybackDurationChanged(qint64 ms) {
    m_playbackTimeLabel->setText(QString("%1 / %2").arg(formatTime(m_player->position()), formatTime(ms)));
}

void MainWindow::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    if (state == QMediaPlayer::PlayingState) {
        m_playBtn->setText(tr("Pause"));
        m_playBtn->setIcon(QIcon(":/icons/pause.svg"));
        m_playBtn->setStyleSheet("background-color: #d97706; color: white; font-weight: 600; min-width: 80px;");
    } else {
        m_playBtn->setText(tr("Play"));
        m_playBtn->setIcon(QIcon(":/icons/play.svg"));
        m_playBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600; min-width: 80px;");
    }
}

void MainWindow::onSpeedChanged(int idx) {
    qreal speed = m_speedCombo->itemData(idx).toReal();
    m_player->setPlaybackRate(speed);
}

void MainWindow::onVolumeSliderChanged(int val) {
    m_player->setVolume(val / 100.0f);
}

void MainWindow::onMuteToggle() {
    bool muted = !m_player->isMuted();
    m_player->setMuted(muted);
    m_muteBtn->setIcon(QIcon(muted ? ":/icons/volume-x.svg" : ":/icons/volume.svg"));
}

void MainWindow::onReplayCurrentSentence() {
    qint64 pos = m_player->position();
    for (const auto &seg : m_currentSession.segments) {
        if (pos >= seg.startMs && pos <= seg.endMs) {
            m_player->playSegment(seg.startMs, seg.endMs, m_loopSentenceBtn->isChecked());
            return;
        }
    }
    if (!m_currentSession.segments.isEmpty()) {
        const auto &first = m_currentSession.segments.first();
        m_player->playSegment(first.startMs, first.endMs, m_loopSentenceBtn->isChecked());
    }
}

void MainWindow::onLoopSentenceToggle(bool checked) {
    m_player->setLoopSegment(checked);
    m_loopSentenceBtn->setStyleSheet(
        checked ? "background-color: #4f46e5; color: white; font-weight: 600; border-color: #818cf8;"
                : "background-color: #1e2129; color: #f8fafc;"
    );
}

// -----------------------------------------------------------------------------
// Timeline & Note Slots
// -----------------------------------------------------------------------------

void MainWindow::onTimelineSeekRequested(qint64 ms) {
    m_player->seek(ms);
}

void MainWindow::onTimelineSegmentClicked(int segmentId, qint64 startMs, qint64) {
    m_activeSentenceId = segmentId;
    m_player->seek(startMs);
}

void MainWindow::onTimelineSegmentDoubleClicked(int segmentId, qint64 startMs, qint64 endMs) {
    m_activeSentenceId = segmentId;
    m_player->playSegment(startMs, endMs, m_loopSentenceBtn->isChecked());
}

void MainWindow::onNoteTimestampClicked(qint64 ms) {
    m_player->seek(ms);
    m_player->play();
}

// -----------------------------------------------------------------------------
// Whisper STT Slots
// -----------------------------------------------------------------------------

void MainWindow::refreshModelList() {
    m_modelCombo->clear();
    auto installed = m_modelManager->installedModels();

    for (const auto &m : installed) {
        m_modelCombo->addItem(m.name, m.filePath);
    }

    if (m_modelCombo->count() == 0) {
        m_modelCombo->addItem(tr("(No models installed)"), QString());
    } else {
        // Select German model by default if present
        for (int i = 0; i < m_modelCombo->count(); ++i) {
            if (m_modelCombo->itemText(i).contains("German", Qt::CaseInsensitive)) {
                m_modelCombo->setCurrentIndex(i);
                break;
            }
        }
        onModelSelectionChanged(m_modelCombo->currentIndex());
    }
}

void MainWindow::onModelSelectionChanged(int idx) {
    QString path = m_modelCombo->itemData(idx).toString();
    if (!path.isEmpty()) {
        m_whisper->setModelPath(path);
        m_currentSession.whisperModel = path;
    }
}

void MainWindow::onTranscribeClicked() {
    if (m_currentAudioPcm.empty()) {
        QMessageBox::warning(this, tr("No Audio"), tr("Please record or open an audio session first."));
        return;
    }

    QString modelPath = m_modelCombo->currentData().toString();
    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        QMessageBox::warning(this, tr("No Model Selected"),
                             tr("Please select or download a Whisper model."));
        setWorkspaceLayout(WorkspaceLayout::Tabbed);
        m_tabWidget->setCurrentIndex(1); // Models tab
        return;
    }

    m_whisper->setModelPath(modelPath);
    m_whisper->setLanguage(m_languageCombo->currentData().toString());
    m_whisper->transcribe(m_currentAudioPcm);
}

void MainWindow::onWhisperStarted() {
    m_transcribeBtn->setEnabled(false);
    m_transcribeProgress->show();
    m_transcribeProgress->setValue(10);
    statusBar()->showMessage(tr("Whisper.cpp transcribing audio..."));
}

void MainWindow::onWhisperProgress(int pct) {
    m_transcribeProgress->setValue(pct);
}

void MainWindow::onWhisperSegmentDiscovered(const AudioSegment &) {
}

void MainWindow::onWhisperCompleted(const QList<AudioSegment> &segments) {
    m_transcribeBtn->setEnabled(true);
    m_transcribeProgress->hide();

    m_currentSession.segments = segments;
    m_sentenceListView->setSegments(segments);
    m_waveformWidget->setSegments(segments);

    onSaveSession();
    statusBar()->showMessage(tr("Transcription finished! %1 sentences transcribed.").arg(segments.size()));
}

void MainWindow::onWhisperFailed(const QString &err) {
    m_transcribeBtn->setEnabled(true);
    m_transcribeProgress->hide();
    QMessageBox::critical(this, tr("Transcription Error"), err);
    statusBar()->showMessage(tr("Transcription error: %1").arg(err));
}

// -----------------------------------------------------------------------------
// Pocket TTS Slots
// -----------------------------------------------------------------------------

void MainWindow::onTtsSentenceRequested(int id, const QString &text) {
    m_requestingTtsSentenceId = id;
    QString outDir = m_sessionManager->getTtsDirectory(m_currentSession.id);
    QString outPath = QString("%1/tts_sentence_%2.wav").arg(outDir).arg(id);

    if (QFile::exists(outPath)) {
        m_player->setSource(outPath);
        m_player->play();
        return;
    }

    QString lang = m_currentSession.language;
    QString voice = m_tts->defaultVoiceForLanguage(lang);
    statusBar()->showMessage(tr("Generating Pocket TTS pronunciation for sentence #%1...").arg(id));
    m_tts->generateSpeech(text, outPath, lang, voice);
}

void MainWindow::onTtsStudioGenerate() {
    QString text = m_ttsInputEdit->text();
    QString voice = m_ttsVoiceCombo->currentData().toString();
    QString outPath = QString("/tmp/pocket_tts_studio_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());

    m_ttsGenerateBtn->setEnabled(false);
    m_ttsStatusLabel->setText(tr("Generating speech..."));

    m_requestingTtsSentenceId = -1;
    m_tts->generateSpeech(text, outPath, "german", voice);
}

void MainWindow::onTtsStarted(const QString &) {
    statusBar()->showMessage(tr("Pocket TTS generating..."));
}

void MainWindow::onTtsCompleted(const QString &outputPath) {
    m_ttsGenerateBtn->setEnabled(true);
    m_lastTtsAudioPath = outputPath;
    m_ttsPlayResultBtn->setEnabled(true);
    m_ttsStatusLabel->setText(tr("Speech generated successfully!"));

    if (m_requestingTtsSentenceId != -1) {
        m_sentenceListView->markTtsAvailable(m_requestingTtsSentenceId, outputPath);
        m_player->setSource(outputPath);
        m_player->play();
        m_requestingTtsSentenceId = -1;
    }

    statusBar()->showMessage(tr("Pocket TTS audio ready: %1").arg(outputPath));
}

void MainWindow::onTtsFailed(const QString &err) {
    m_ttsGenerateBtn->setEnabled(true);
    m_ttsStatusLabel->setText(tr("Error: %1").arg(err));
    statusBar()->showMessage(tr("TTS generation failed: %1").arg(err));
}

// -----------------------------------------------------------------------------
// Session Slots
// -----------------------------------------------------------------------------

void MainWindow::onNewSession() {
    m_currentSession = m_sessionManager->createNewSession();
    m_currentAudioPcm.clear();
    loadCurrentSessionData();
    updateSessionList();
}

void MainWindow::onSaveSession() {
    m_currentSession.title = m_sessionTitleEdit->text();
    m_currentSession.noteMarkdown = m_noteEditor->markdownText();
    m_currentSession.segments = m_sentenceListView->segments();

    m_sessionManager->saveSession(m_currentSession, m_currentAudioPcm);
    statusBar()->showMessage(tr("Session saved: %1").arg(m_currentSession.title));
}

void MainWindow::onDeleteSession() {
    int row = m_sessionListWidget->currentRow();
    if (row < 0) return;

    auto reply = QMessageBox::question(
        this, tr("Delete Session"),
        tr("Are you sure you want to delete session '%1'?").arg(m_currentSession.title),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_sessionManager->deleteSession(m_currentSession.id);
        updateSessionList();
        if (m_sessionListWidget->count() > 0) {
            m_sessionListWidget->setCurrentRow(0);
        } else {
            onNewSession();
        }
    }
}

void MainWindow::onSessionSelected(int row) {
    if (row < 0) return;
    auto sessions = m_sessionManager->listSessions();
    if (row < sessions.size()) {
        m_sessionManager->loadSession(sessions[row].id, m_currentSession, m_currentAudioPcm);
        loadCurrentSessionData();
    }
}

void MainWindow::loadCurrentSessionData() {
    m_sessionTitleEdit->setText(m_currentSession.title);
    m_noteEditor->setMarkdownText(m_currentSession.noteMarkdown);
    m_sentenceListView->setSegments(m_currentSession.segments);
    m_waveformWidget->setAudioData(m_currentAudioPcm, m_currentSession.durationMs);
    m_waveformWidget->setSegments(m_currentSession.segments);

    QString audioPath = m_sessionManager->getAudioPath(m_currentSession.id);
    m_player->setSource(QFile::exists(audioPath) ? audioPath : QString());

    m_recordingTimeLabel->setText(formatTime(m_currentSession.durationMs));
}

void MainWindow::updateSessionList() {
    m_sessionListWidget->blockSignals(true);
    m_sessionListWidget->clear();

    auto sessions = m_sessionManager->listSessions();
    for (const auto &s : sessions) {
        QString text = QString("%1\n%2 (%3)")
            .arg(s.title)
            .arg(s.createdAt.toString("yyyy-MM-dd hh:mm"))
            .arg(formatTime(s.durationMs));
        m_sessionListWidget->addItem(text);
    }
    m_sessionListWidget->blockSignals(false);
}

// -----------------------------------------------------------------------------
// Model Manager UI Slots
// -----------------------------------------------------------------------------

void MainWindow::onDownloadPresetRequested(const QString &presetId) {
    m_modelManager->downloadPreset(presetId);
}

void MainWindow::onAddCustomModelClicked() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Select Whisper Model"),
        QDir::homePath(), tr("Whisper Models (*.bin *.gguf);;All Files (*)")
    );
    if (!path.isEmpty()) {
        m_modelManager->addCustomModel(path);
    }
}

void MainWindow::onConvertHfModelClicked() {
    QString hfId = m_hfInputEdit->text().trimmed();
    if (hfId.isEmpty()) return;

    statusBar()->showMessage(tr("Converting %1 to GGML...").arg(hfId));
    m_modelManager->convertHuggingFaceModel(hfId);
}

QString MainWindow::formatTime(qint64 ms) const {
    int totalSec = static_cast<int>(ms / 1000);
    int minutes = totalSec / 60;
    int seconds = totalSec % 60;
    int tenths = static_cast<int>((ms % 1000) / 100);
    return QString("%1:%2.%3")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(tenths);
}
