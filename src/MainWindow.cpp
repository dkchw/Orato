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
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Recorder — Qt6 Speech & Pronunciation Studio"));
    resize(1280, 840);

    // Dark theme palette / style
    setStyleSheet(
        "QMainWindow { background-color: #121316; color: #f1f2f6; }"
        "QWidget { color: #f1f2f6; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }"
        "QGroupBox { font-weight: bold; border: 1px solid #2b2e38; border-radius: 6px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #70a1ff; }"
        "QPushButton { background-color: #2b2e38; border: 1px solid #3b3f4d; border-radius: 4px; padding: 5px 12px; color: #f1f2f6; font-size: 12px; }"
        "QPushButton:hover { background-color: #3b3f4d; border-color: #5352ed; }"
        "QPushButton:pressed { background-color: #1e2026; }"
        "QComboBox { background-color: #20222a; border: 1px solid #3b3f4d; border-radius: 4px; padding: 4px 8px; color: #f1f2f6; min-height: 22px; }"
        "QComboBox::drop-down { border: none; width: 18px; }"
        "QComboBox QAbstractItemView { background-color: #20222a; color: #f1f2f6; selection-background-color: #3742fa; }"
        "QLineEdit { background-color: #20222a; border: 1px solid #3b3f4d; border-radius: 4px; padding: 5px 8px; color: #f1f2f6; }"
        "QLineEdit:focus { border-color: #70a1ff; }"
        "QSlider::groove:horizontal { height: 6px; background: #2b2e38; border-radius: 3px; }"
        "QSlider::sub-page:horizontal { background: #3742fa; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #70a1ff; width: 14px; margin-top: -4px; margin-bottom: -4px; border-radius: 7px; }"
        "QTabWidget::pane { border: 1px solid #2b2e38; background-color: #18191e; border-radius: 4px; }"
        "QTabBar::tab { background: #20222a; color: #a4b0be; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; font-weight: bold; }"
        "QTabBar::tab:selected { background: #18191e; color: #70a1ff; border: 1px solid #2b2e38; border-bottom: none; }"
        "QListWidget { background-color: #18191e; border: 1px solid #2b2e38; border-radius: 4px; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #23252d; }"
        "QListWidget::item:selected { background-color: #2b2e38; color: #70a1ff; }"
        "QStatusBar { background-color: #18191e; color: #a4b0be; border-top: 1px solid #23252d; }"
    );

    // Initialize core engines
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

    // Initialize state
    refreshAudioDevices();
    refreshModelList();
    updateSessionList();

    // Load or create initial session
    auto sessions = m_sessionManager->listSessions();
    if (!sessions.isEmpty()) {
        m_sessionListWidget->setCurrentRow(0);
    } else {
        onNewSession();
    }

    statusBar()->showMessage(tr("Ready. Choose audio source and click Record or Open a session."));
}

void MainWindow::setupUi() {
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // Left Sidebar: Session List
    auto *sidebarWidget = new QWidget(this);
    setupSidebar(sidebarWidget);
    mainLayout->addWidget(sidebarWidget, 1);

    // Right Area: Main Studio
    auto *studioWidget = new QWidget(this);
    auto *studioLayout = new QVBoxLayout(studioWidget);
    studioLayout->setContentsMargins(0, 0, 0, 0);
    studioLayout->setSpacing(6);

    // 1. Top Bar (Recording + Whisper controls)
    auto *topBarWidget = new QWidget(this);
    setupTopBar(topBarWidget);
    studioLayout->addWidget(topBarWidget);

    // 2. Waveform & Timeline Area
    auto *waveformContainer = new QWidget(this);
    setupWaveformArea(waveformContainer);
    studioLayout->addWidget(waveformContainer);

    // 3. Tabbed Workspace (Sentences, Notes, TTS Studio, Model Manager)
    m_tabWidget = new QTabWidget(this);

    // Tab 1: Sentences View
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
        m_tabWidget->setCurrentWidget(m_noteEditor);
    });
    connect(m_sentenceListView, &SentenceListView::segmentsChanged, this, [this]() {
        m_currentSession.segments = m_sentenceListView->segments();
        m_waveformWidget->setSegments(m_currentSession.segments);
    });
    m_tabWidget->addTab(m_sentenceListView, tr("📝 Transcribed Sentences"));

    // Tab 2: Markdown Note Editor
    m_noteEditor = new MarkdownNoteEditor(this);
    connect(m_noteEditor, &MarkdownNoteEditor::textChanged, this, [this]() {
        m_currentSession.noteMarkdown = m_noteEditor->markdownText();
    });
    connect(m_noteEditor, &MarkdownNoteEditor::timestampClicked,
            this, &MainWindow::onNoteTimestampClicked);
    m_tabWidget->addTab(m_noteEditor, tr("📓 Session Notes (Markdown)"));

    // Tab 3: Pocket TTS Studio
    auto *ttsTabWidget = new QWidget(this);
    setupTtsStudioTab(ttsTabWidget);
    m_tabWidget->addTab(ttsTabWidget, tr("🔊 Pocket TTS Studio"));

    // Tab 4: Whisper Models Manager
    auto *modelTabWidget = new QWidget(this);
    setupModelManagerTab(modelTabWidget);
    m_tabWidget->addTab(modelTabWidget, tr("🧠 Whisper Models"));

    studioLayout->addWidget(m_tabWidget, 1);

    // 4. Bottom Bar (Full-feature Playback Controls)
    auto *bottomBarWidget = new QWidget(this);
    setupBottomBar(bottomBarWidget);
    studioLayout->addWidget(bottomBarWidget);

    mainLayout->addWidget(studioWidget, 4);

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
}

void MainWindow::setupSidebar(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    auto *titleLabel = new QLabel(tr("Training Sessions"), container);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #70a1ff;");
    layout->addWidget(titleLabel);

    auto *btnRow = new QHBoxLayout();
    auto *newBtn = new QPushButton(tr("➕ New"), container);
    newBtn->setStyleSheet("background-color: #2ed573; color: #1e272e; font-weight: bold;");
    connect(newBtn, &QPushButton::clicked, this, &MainWindow::onNewSession);
    btnRow->addWidget(newBtn);

    auto *saveBtn = new QPushButton(tr("💾 Save"), container);
    saveBtn->setStyleSheet("background-color: #3742fa; color: white; font-weight: bold;");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveSession);
    btnRow->addWidget(saveBtn);

    auto *delBtn = new QPushButton(tr("🗑"), container);
    delBtn->setToolTip(tr("Delete Session"));
    delBtn->setStyleSheet("background-color: #ff4757; color: white;");
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::onDeleteSession);
    btnRow->addWidget(delBtn);

    layout->addLayout(btnRow);

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
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    // Audio Input device combo
    topRow->addWidget(new QLabel(tr("🎙️ Input:"), container));
    m_inputDeviceCombo = new QComboBox(container);
    m_inputDeviceCombo->setMinimumWidth(180);
    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAudioInputDeviceChanged);
    topRow->addWidget(m_inputDeviceCombo);

    auto *refreshDevBtn = new QPushButton(tr("↻"), container);
    refreshDevBtn->setToolTip(tr("Refresh Audio Devices"));
    refreshDevBtn->setFixedWidth(28);
    connect(refreshDevBtn, &QPushButton::clicked, this, &MainWindow::refreshAudioDevices);
    topRow->addWidget(refreshDevBtn);

    // Record / Pause buttons
    m_recordBtn = new QPushButton(tr("🔴 Record"), container);
    m_recordBtn->setStyleSheet("background-color: #ff4757; color: white; font-weight: bold; font-size: 13px; padding: 6px 14px;");
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::onRecordToggle);
    topRow->addWidget(m_recordBtn);

    m_pauseBtn = new QPushButton(tr("⏸"), container);
    m_pauseBtn->setEnabled(false);
    connect(m_pauseBtn, &QPushButton::clicked, this, &MainWindow::onPauseToggle);
    topRow->addWidget(m_pauseBtn);

    m_recordingTimeLabel = new QLabel("00:00", container);
    m_recordingTimeLabel->setStyleSheet("font-family: monospace; font-size: 14px; font-weight: bold; color: #ff6b81;");
    topRow->addWidget(m_recordingTimeLabel);

    // VU Meter
    m_levelMeter = new AudioLevelMeter(container);
    m_levelMeter->setFixedWidth(90);
    topRow->addWidget(m_levelMeter);

    topRow->addSpacing(15);

    // Whisper Model Selection
    topRow->addWidget(new QLabel(tr("Model:"), container));
    m_modelCombo = new QComboBox(container);
    m_modelCombo->setMinimumWidth(190);
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
    m_transcribeBtn = new QPushButton(tr("⚡ Transcribe"), container);
    m_transcribeBtn->setStyleSheet("background-color: #5352ed; color: white; font-weight: bold; padding: 6px 14px;");
    connect(m_transcribeBtn, &QPushButton::clicked, this, &MainWindow::onTranscribeClicked);
    topRow->addWidget(m_transcribeBtn);

    topRow->addStretch();
    mainLayout->addLayout(topRow);

    // Transcribe Progress Bar (hidden by default)
    m_transcribeProgress = new QProgressBar(container);
    m_transcribeProgress->setRange(0, 100);
    m_transcribeProgress->setValue(0);
    m_transcribeProgress->setFixedHeight(6);
    m_transcribeProgress->setTextVisible(false);
    m_transcribeProgress->setStyleSheet(
        "QProgressBar { background: #20222a; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #70a1ff, stop:1 #2ed573); border-radius: 3px; }"
    );
    m_transcribeProgress->hide();
    mainLayout->addWidget(m_transcribeProgress);
}

void MainWindow::setupWaveformArea(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // Waveform Top Controls (Zoom In, Zoom Out, Fit)
    auto *topRow = new QHBoxLayout();
    topRow->setContentsMargins(4, 0, 4, 0);

    auto *timelineTitle = new QLabel(tr("Visual Timeline"), container);
    timelineTitle->setStyleSheet("font-size: 11px; font-weight: bold; color: #70a1ff;");
    topRow->addWidget(timelineTitle);

    topRow->addStretch();

    auto *zoomInBtn = new QPushButton(tr("🔍 +"), container);
    zoomInBtn->setFixedWidth(36);
    connect(zoomInBtn, &QPushButton::clicked, this, [this]() { m_waveformWidget->zoomIn(); });
    topRow->addWidget(zoomInBtn);

    auto *zoomOutBtn = new QPushButton(tr("🔍 -"), container);
    zoomOutBtn->setFixedWidth(36);
    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() { m_waveformWidget->zoomOut(); });
    topRow->addWidget(zoomOutBtn);

    auto *zoomFitBtn = new QPushButton(tr("Fit"), container);
    zoomFitBtn->setFixedWidth(36);
    connect(zoomFitBtn, &QPushButton::clicked, this, [this]() { m_waveformWidget->zoomFit(); });
    topRow->addWidget(zoomFitBtn);

    layout->addLayout(topRow);

    // Interactive Waveform Widget
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

    // Top Row: Slider and timestamps
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
    m_playbackTimeLabel->setStyleSheet("font-family: monospace; font-size: 12px; color: #a4b0be;");
    sliderRow->addWidget(m_playbackTimeLabel);

    layout->addLayout(sliderRow);

    // Bottom Row: Playback buttons
    auto *ctrlRow = new QHBoxLayout();
    ctrlRow->setSpacing(6);

    m_playBtn = new QPushButton(tr("▶ Play"), container);
    m_playBtn->setStyleSheet("background-color: #2ed573; color: #1e272e; font-weight: bold; min-width: 70px;");
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayToggle);
    ctrlRow->addWidget(m_playBtn);

    m_stopBtn = new QPushButton(tr("⏹"), container);
    m_stopBtn->setToolTip(tr("Stop playback"));
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    ctrlRow->addWidget(m_stopBtn);

    m_skipBackBtn = new QPushButton(tr("⏪ -5s"), container);
    connect(m_skipBackBtn, &QPushButton::clicked, this, [this]() { m_player->skipBackward(5000); });
    ctrlRow->addWidget(m_skipBackBtn);

    m_skipForwardBtn = new QPushButton(tr("+5s ⏩"), container);
    connect(m_skipForwardBtn, &QPushButton::clicked, this, [this]() { m_player->skipForward(5000); });
    ctrlRow->addWidget(m_skipForwardBtn);

    ctrlRow->addSpacing(10);

    m_replaySentenceBtn = new QPushButton(tr("🔄 Replay Sentence [R]"), container);
    m_replaySentenceBtn->setStyleSheet("background-color: #3742fa; color: white; font-weight: bold;");
    connect(m_replaySentenceBtn, &QPushButton::clicked, this, &MainWindow::onReplayCurrentSentence);
    ctrlRow->addWidget(m_replaySentenceBtn);

    m_loopSentenceBtn = new QPushButton(tr("🔁 Loop Sentence"), container);
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
    m_muteBtn = new QPushButton(tr("🔊"), container);
    m_muteBtn->setFixedWidth(30);
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
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *descLabel = new QLabel(
        tr("<b>Kyutai Pocket TTS Studio:</b> Generate natural speech on CPU for language learning and pronunciation comparison."),
        container
    );
    descLabel->setStyleSheet("color: #70a1ff;");
    layout->addWidget(descLabel);

    auto *inputGroup = new QGroupBox(tr("Text to Speech"), container);
    auto *groupLayout = new QVBoxLayout(inputGroup);

    m_ttsInputEdit = new QLineEdit(inputGroup);
    m_ttsInputEdit->setPlaceholderText(tr("Enter text to speak with Pocket TTS (e.g. 'Hallo, wie geht es dir heute?')..."));
    m_ttsInputEdit->setText("Guten Tag! Ich lerne heute Deutsch mit dem Recorder Studio.");
    groupLayout->addWidget(m_ttsInputEdit);

    auto *optRow = new QHBoxLayout();
    optRow->addWidget(new QLabel(tr("Voice:"), inputGroup));
    m_ttsVoiceCombo = new QComboBox(inputGroup);
    for (const auto &v : m_tts->availableVoices()) {
        m_ttsVoiceCombo->addItem(v.displayName, v.id);
    }
    optRow->addWidget(m_ttsVoiceCombo);

    m_ttsGenerateBtn = new QPushButton(tr("⚡ Generate Speech (Pocket TTS)"), inputGroup);
    m_ttsGenerateBtn->setStyleSheet("background-color: #ffa502; color: #1e272e; font-weight: bold;");
    connect(m_ttsGenerateBtn, &QPushButton::clicked, this, &MainWindow::onTtsStudioGenerate);
    optRow->addWidget(m_ttsGenerateBtn);

    m_ttsPlayResultBtn = new QPushButton(tr("▶ Play Generated Speech"), inputGroup);
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

    m_ttsStatusLabel = new QLabel(tr("Ready. Pocket TTS model loaded locally."), inputGroup);
    m_ttsStatusLabel->setStyleSheet("color: #a4b0be; font-size: 11px;");
    groupLayout->addWidget(m_ttsStatusLabel);

    layout->addWidget(inputGroup);
    layout->addStretch();
}

void MainWindow::setupModelManagerTab(QWidget *container) {
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *hdr = new QLabel(
        tr("<b>Whisper.cpp Models Manager:</b> Download preset models (including fine-tuned German) or convert custom HuggingFace models."),
        container
    );
    hdr->setStyleSheet("color: #70a1ff;");
    layout->addWidget(hdr);

    // Preset models table
    auto *table = new QTableWidget(container);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({tr("Model Name"), tr("Language"), tr("Size"), tr("Action")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->setStyleSheet("QTableWidget { background-color: #1e2027; border: 1px solid #363945; }");

    auto presets = m_modelManager->presetModels();
    table->setRowCount(presets.size());

    for (int r = 0; r < presets.size(); ++r) {
        const auto &p = presets[r];
        table->setItem(r, 0, new QTableWidgetItem(p.name));
        table->setItem(r, 1, new QTableWidgetItem(p.language.toUpper()));
        table->setItem(r, 2, new QTableWidgetItem(QString("%1 MB").arg(p.approxSizeMb)));

        bool installed = m_modelManager->isModelInstalled(p.id);
        auto *actionBtn = new QPushButton(installed ? tr("✓ Installed") : tr("⬇ Download"), table);
        if (installed) {
            actionBtn->setEnabled(false);
            actionBtn->setStyleSheet("background-color: #2ed573; color: #1e272e; font-weight: bold;");
        } else {
            actionBtn->setStyleSheet("background-color: #3742fa; color: white;");
            QString pid = p.id;
            connect(actionBtn, &QPushButton::clicked, this, [this, pid]() {
                onDownloadPresetRequested(pid);
            });
        }
        table->setCellWidget(r, 3, actionBtn);
    }
    layout->addWidget(table, 1);

    // Status & Progress
    m_modelDownloadStatusLabel = new QLabel(container);
    m_modelDownloadStatusLabel->setStyleSheet("color: #70a1ff; font-size: 11px;");
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
        m_modelDownloadStatusLabel->setText(tr("Successfully downloaded model to %1").arg(path));
        m_modelDownloadProgressBar->hide();
        refreshModelList();
    });
    connect(m_modelManager, &ModelManager::downloadFailed, this, [this](const QString &err) {
        m_modelDownloadStatusLabel->setText(tr("Download failed: %1").arg(err));
        m_modelDownloadProgressBar->hide();
    });

    // Custom Model & HF converter
    auto *bottomRow = new QHBoxLayout();
    auto *addCustomBtn = new QPushButton(tr("📁 Browse Custom Model (.bin/.gguf)..."), container);
    connect(addCustomBtn, &QPushButton::clicked, this, &MainWindow::onAddCustomModelClicked);
    bottomRow->addWidget(addCustomBtn);

    bottomRow->addSpacing(20);

    bottomRow->addWidget(new QLabel(tr("HuggingFace Model:"), container));
    m_hfInputEdit = new QLineEdit("https://huggingface.co/primeline/whisper-tiny-german", container);
    bottomRow->addWidget(m_hfInputEdit, 1);

    auto *convertBtn = new QPushButton(tr("Convert HF to GGML"), container);
    connect(convertBtn, &QPushButton::clicked, this, &MainWindow::onConvertHfModelClicked);
    bottomRow->addWidget(convertBtn);

    layout->addLayout(bottomRow);
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
    // Selected device will be used on next record
}

void MainWindow::onRecordToggle() {
    if (m_recorder->state() == AudioRecorder::State::Recording) {
        QString audioPath = m_sessionManager->getAudioPath(m_currentSession.id);
        m_recorder->stopRecording(audioPath);
        m_recordBtn->setText(tr("🔴 Record"));
        m_recordBtn->setStyleSheet("background-color: #ff4757; color: white; font-weight: bold; font-size: 13px; padding: 6px 14px;");
        m_pauseBtn->setEnabled(false);
    } else {
        QVariant data = m_inputDeviceCombo->currentData();
        QAudioDevice dev = data.value<QAudioDevice>();

        if (m_recorder->startRecording(dev)) {
            m_recordBtn->setText(tr("⏹ Stop"));
            m_recordBtn->setStyleSheet("background-color: #e74c3c; color: yellow; font-weight: bold; font-size: 13px; padding: 6px 14px; border: 2px solid yellow;");
            m_pauseBtn->setEnabled(true);
            m_pauseBtn->setText(tr("⏸"));
            statusBar()->showMessage(tr("Recording..."));
        }
    }
}

void MainWindow::onPauseToggle() {
    if (m_recorder->state() == AudioRecorder::State::Recording) {
        m_recorder->pauseRecording();
        m_pauseBtn->setText(tr("▶ Resume"));
        statusBar()->showMessage(tr("Recording paused."));
    } else if (m_recorder->state() == AudioRecorder::State::Paused) {
        m_recorder->resumeRecording();
        m_pauseBtn->setText(tr("⏸"));
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
    statusBar()->showMessage(tr("Recording complete. Duration: %1. Ready to transcribe!").arg(formatTime(durationMs)));

    // Offer to transcribe automatically
    auto reply = QMessageBox::question(
        this, tr("Recording Complete"),
        tr("Audio recorded successfully (%1).\nWould you like to transcribe it now with Whisper?").arg(formatTime(durationMs)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
    );
    if (reply == QMessageBox::Yes) {
        onTranscribeClicked();
    }
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
        m_playBtn->setText(tr("⏸ Pause"));
        m_playBtn->setStyleSheet("background-color: #ffa502; color: #1e272e; font-weight: bold; min-width: 70px;");
    } else {
        m_playBtn->setText(tr("▶ Play"));
        m_playBtn->setStyleSheet("background-color: #2ed573; color: #1e272e; font-weight: bold; min-width: 70px;");
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
    m_muteBtn->setText(muted ? tr("🔇") : tr("🔊"));
}

void MainWindow::onReplayCurrentSentence() {
    qint64 pos = m_player->position();
    for (const auto &seg : m_currentSession.segments) {
        if (pos >= seg.startMs && pos <= seg.endMs) {
            m_player->playSegment(seg.startMs, seg.endMs, m_loopSentenceBtn->isChecked());
            return;
        }
    }
    // If not in a segment, replay the first segment after or before
    if (!m_currentSession.segments.isEmpty()) {
        const auto &first = m_currentSession.segments.first();
        m_player->playSegment(first.startMs, first.endMs, m_loopSentenceBtn->isChecked());
    }
}

void MainWindow::onLoopSentenceToggle(bool checked) {
    m_player->setLoopSegment(checked);
    m_loopSentenceBtn->setStyleSheet(
        checked ? "background-color: #2ed573; color: #1e272e; font-weight: bold;"
                : "background-color: #2b2e38; color: #f1f2f6;"
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
                             tr("Please select or download a Whisper model in the 'Whisper Models' tab."));
        m_tabWidget->setCurrentIndex(3);
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

void MainWindow::onWhisperSegmentDiscovered(const AudioSegment &seg) {
    // Add dynamically to session segments
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

    // If already generated and cached, play immediately!
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
    m_ttsStatusLabel->setText(tr("✓ Speech generated successfully!"));

    if (m_requestingTtsSentenceId != -1) {
        m_sentenceListView->markTtsAvailable(m_requestingTtsSentenceId, outputPath);
        // Automatically play the generated pronunciation
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
