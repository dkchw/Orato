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
#include <QEvent>
#include <QResizeEvent>
#include "widgets/ModelManagerDialog.h"
#include "widgets/ConfigDialog.h"
#include "widgets/SessionItemDelegate.h"
#include "widgets/FlowLayout.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Orato — Speech & Pronunciation Studio"));
    resize(1320, 880);
    setMinimumSize(780, 520);

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
        "QPushButton { background-color: #1e2129; border: 1px solid #333846; border-radius: 5px; padding: 4px 8px; color: #f8fafc; font-size: 12px; font-weight: 500; }"
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

    // Background Model Preloading (Zero Cold Start)
    QString defaultModelPath;
    for (const auto &p : m_modelManager->presetModels()) {
        if (m_modelManager->isModelInstalled(p.id)) {
            defaultModelPath = m_modelManager->getModelPath(p.id);
            break;
        }
    }
    if (defaultModelPath.isEmpty()) {
        auto models = m_modelManager->installedModels();
        if (!models.isEmpty()) {
            defaultModelPath = models.first().filePath;
        }
    }
    if (!defaultModelPath.isEmpty()) {
        m_whisper->preloadModel(defaultModelPath);
    }

    // Load initial session
    auto sessions = m_sessionManager->listSessions();
    if (!sessions.isEmpty()) {
        m_sessionListWidget->setCurrentRow(0);
    } else {
        onNewSession();
    }

    statusBar()->showMessage(tr("Ready. Whisper & Pocket TTS preloaded."));
}

void MainWindow::setupUi() {
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(0);

    // Resizable Main Splitter (Left Sidebar + Right Studio)
    m_mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    m_mainSplitter->setChildrenCollapsible(false); // Smooth, continuous resizing without snapping to 0
    m_mainSplitter->setHandleWidth(6);

    // 1. Sidebar (Sessions panel)
    m_sidebarWidget = new QWidget(m_mainSplitter);
    m_sidebarWidget->setMinimumWidth(220);
    setupSidebar(m_sidebarWidget);
    m_mainSplitter->addWidget(m_sidebarWidget);

    // 2. Right Studio Area
    m_studioWidget = new QWidget(m_mainSplitter);
    m_studioWidget->setMinimumWidth(450);
    auto *studioLayout = new QVBoxLayout(m_studioWidget);
    studioLayout->setContentsMargins(6, 4, 4, 4);
    studioLayout->setSpacing(6);

    // Top Bar (Recording + STT controls)
    m_topBarWidget = new QWidget(m_studioWidget);
    setupTopBar(m_topBarWidget);
    studioLayout->addWidget(m_topBarWidget);

    // Waveform Timeline Area
    auto *waveformContainer = new QWidget(m_studioWidget);
    setupWaveformArea(waveformContainer);
    studioLayout->addWidget(waveformContainer);

    // Workspace Area: Stacked widget containing Splitter mode (Note + Transcript) or Tabs mode
    m_workspaceStack = new QStackedWidget(m_studioWidget);

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
    auto *bottomBarWidget = new QWidget(m_studioWidget);
    setupBottomBar(bottomBarWidget);
    studioLayout->addWidget(bottomBarWidget);

    m_mainSplitter->addWidget(m_studioWidget);

    // Default splitter proportions (Sidebar: 300px, Studio: 1020px)
    m_mainSplitter->setSizes({300, 1020});

    mainLayout->addWidget(m_mainSplitter);

    connect(m_mainSplitter, &QSplitter::splitterMoved, this, [this]() {
        if (m_flowContainer) {
            m_flowContainer->updateGeometry();
        }
        if (m_topBarWidget) {
            m_topBarWidget->updateGeometry();
            if (m_topBarWidget->parentWidget() && m_topBarWidget->parentWidget()->layout()) {
                m_topBarWidget->parentWidget()->layout()->activate();
            }
        }
        m_sessionListWidget->doItemsLayout();
    });

    // Connect core recorder signals
    connect(m_recorder, &AudioRecorder::durationChanged, this, &MainWindow::onRecordingDurationChanged);
    connect(m_recorder, &AudioRecorder::levelChanged, m_levelMeter, &AudioLevelMeter::setLevels);
    connect(m_recorder, &AudioRecorder::recordingFinished, this, &MainWindow::onRecordingFinished);
    connect(m_recorder, &AudioRecorder::liveAudioUpdated, this, [this](const std::vector<float> &liveSamples, qint64 durMs) {
        m_currentAudioPcm = liveSamples;
        m_waveformWidget->setLiveAudioData(liveSamples, durMs);
        m_recordingTimeLabel->setText(formatTime(durMs));
    });

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

    // Dock Side Button (Toggle Left / Right placement)
    m_dockSideBtn = new QPushButton(container);
    m_dockSideBtn->setIcon(QIcon(":/icons/dock.svg"));
    m_dockSideBtn->setToolTip(tr("Dock Sessions Panel to Right / Left"));
    m_dockSideBtn->setFixedSize(28, 28);
    connect(m_dockSideBtn, &QPushButton::clicked, this, &MainWindow::onToggleSidebarDockSide);
    headerRow->addWidget(m_dockSideBtn);

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

    // Sessions List with custom item delegate (full word-wrapping, dynamic height, zero clipping)
    m_sessionListWidget = new QListWidget(container);
    m_sessionListWidget->setItemDelegate(new SessionItemDelegate(m_sessionListWidget));
    m_sessionListWidget->setResizeMode(QListView::Adjust);
    m_sessionListWidget->setWordWrap(true);
    m_sessionListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_sessionListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_sessionListWidget->installEventFilter(this);
    m_sessionListWidget->setStyleSheet(
        "QListWidget { background-color: #14161c; border: 1px solid #272a34; border-radius: 6px; outline: none; padding: 2px; }"
        "QListWidget::item { border: none; margin: 0; }"
    );
    connect(m_sessionListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onSessionSelected);
    layout->addWidget(m_sessionListWidget, 1);

    auto *titleLbl = new QLabel(tr("Session Title:"), container);
    titleLbl->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 600; margin-top: 4px;");
    layout->addWidget(titleLbl);

    m_sessionTitleEdit = new QLineEdit(container);
    m_sessionTitleEdit->setPlaceholderText(tr("Type session title..."));
    connect(m_sessionTitleEdit, &QLineEdit::textEdited, this, [this](const QString &t) {
        m_currentSession.title = t;
        if (m_topSessionTitleEdit && m_topSessionTitleEdit->text() != t) {
            m_topSessionTitleEdit->setText(t);
        }
        int row = m_sessionListWidget->currentRow();
        if (row >= 0 && row < m_sessionListWidget->count()) {
            auto *item = m_sessionListWidget->item(row);
            item->setData(Qt::DisplayRole, t.trimmed().isEmpty() ? tr("Untitled Session") : t);
            item->setToolTip(QString("%1\nCreated: %2\nDuration: %3")
                .arg(t)
                .arg(m_currentSession.createdAt.toString("yyyy-MM-dd hh:mm:ss"))
                .arg(formatTime(m_currentSession.durationMs)));
            m_sessionListWidget->doItemsLayout();
        }
    });
    layout->addWidget(m_sessionTitleEdit);
}

void MainWindow::setupTopBar(QWidget *container) {
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // =========================================================================
    // Row 1: Dedicated Full-Width Session Header
    // =========================================================================
    auto *sessionHeaderRow = new QHBoxLayout();
    sessionHeaderRow->setContentsMargins(0, 0, 0, 0);
    sessionHeaderRow->setSpacing(6);

    // Sidebar toggle button
    m_sidebarToggleBtn = new QPushButton(container);
    m_sidebarToggleBtn->setIcon(QIcon(":/icons/sidebar.svg"));
    m_sidebarToggleBtn->setToolTip(tr("Toggle Left Sidebar [Ctrl+B]"));
    m_sidebarToggleBtn->setFixedSize(28, 28);
    connect(m_sidebarToggleBtn, &QPushButton::clicked, this, &MainWindow::onSidebarToggle);
    sessionHeaderRow->addWidget(m_sidebarToggleBtn);

    // Prominent full Session Title editor (stretches to fill all available width)
    m_topSessionTitleEdit = new QLineEdit(container);
    m_topSessionTitleEdit->setPlaceholderText(tr("Untitled Session"));
    m_topSessionTitleEdit->setStyleSheet(
        "QLineEdit { background: #181b24; border: 1px solid #2d3242; border-radius: 5px; padding: 3px 8px; "
        "font-size: 13px; font-weight: 700; color: #f8fafc; min-width: 120px; }"
        "QLineEdit:focus { border-color: #6366f1; background: #202431; }"
    );
    m_topSessionTitleEdit->setToolTip(tr("Current session name. Click to edit."));
    connect(m_topSessionTitleEdit, &QLineEdit::textEdited, this, [this](const QString &t) {
        m_currentSession.title = t;
        if (m_sessionTitleEdit && m_sessionTitleEdit->text() != t) {
            m_sessionTitleEdit->setText(t);
        }
        int row = m_sessionListWidget->currentRow();
        if (row >= 0 && row < m_sessionListWidget->count()) {
            auto *item = m_sessionListWidget->item(row);
            item->setData(Qt::DisplayRole, t.trimmed().isEmpty() ? tr("Untitled Session") : t);
            item->setToolTip(QString("%1\nCreated: %2\nDuration: %3")
                .arg(t)
                .arg(m_currentSession.createdAt.toString("yyyy-MM-dd hh:mm:ss"))
                .arg(formatTime(m_currentSession.durationMs)));
            m_sessionListWidget->doItemsLayout();
        }
    });
    sessionHeaderRow->addWidget(m_topSessionTitleEdit, 1);

    // Date Badge
    m_sessionDateBadge = new QLabel(container);
    m_sessionDateBadge->setStyleSheet(
        "background: #1e2230; color: #94a3b8; border: 1px solid #2a3042; border-radius: 4px; padding: 2px 6px; font-size: 11px;"
    );
    sessionHeaderRow->addWidget(m_sessionDateBadge);

    // Take Status Badge
    m_sessionTakeBadge = new QLabel(container);
    m_sessionTakeBadge->setStyleSheet(
        "background: #1e2230; color: #a5b4fc; border: 1px solid #3730a3; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: 600;"
    );
    sessionHeaderRow->addWidget(m_sessionTakeBadge);

    mainLayout->addLayout(sessionHeaderRow);

    // =========================================================================
    // Row 2: Dynamic Wrapping Studio Toolbar (FlowLayout)
    // In fullscreen: fits on 1 single row, saving maximum vertical space.
    // In windowed mode: moves up & down lines automatically without compressing.
    // =========================================================================
    m_flowContainer = new QWidget(container);
    m_flowContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_flowContainer->installEventFilter(this);
    auto *flowLayout = new FlowLayout(m_flowContainer, 0, 6, 4);

    // Cluster 1: Audio Input
    auto *inputCluster = new QWidget(m_flowContainer);
    auto *inputLay = new QHBoxLayout(inputCluster);
    inputLay->setContentsMargins(0, 0, 0, 0);
    inputLay->setSpacing(4);

    auto *inputLbl = new QLabel(tr("Input:"), inputCluster);
    inputLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
    inputLay->addWidget(inputLbl);

    m_inputDeviceCombo = new QComboBox(inputCluster);
    m_inputDeviceCombo->setMinimumWidth(95);
    m_inputDeviceCombo->setMaximumWidth(150);
    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAudioInputDeviceChanged);
    inputLay->addWidget(m_inputDeviceCombo);

    auto *refreshDevBtn = new QPushButton(inputCluster);
    refreshDevBtn->setIcon(QIcon(":/icons/refresh-cw.svg"));
    refreshDevBtn->setToolTip(tr("Refresh Audio Devices"));
    refreshDevBtn->setFixedSize(26, 26);
    connect(refreshDevBtn, &QPushButton::clicked, this, &MainWindow::refreshAudioDevices);
    inputLay->addWidget(refreshDevBtn);
    flowLayout->addWidget(inputCluster);

    // Cluster 2: Recording Actions
    auto *recordCluster = new QWidget(m_flowContainer);
    auto *recLay = new QHBoxLayout(recordCluster);
    recLay->setContentsMargins(0, 0, 0, 0);
    recLay->setSpacing(4);

    m_recordBtn = new QPushButton(tr("Record"), recordCluster);
    m_recordBtn->setIcon(QIcon(":/icons/record.svg"));
    m_recordBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: 600; padding: 3px 8px; min-height: 24px;");
    m_recordBtn->setToolTip(tr("Record a fresh take (replaces previous audio)"));
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::onRecordToggle);
    recLay->addWidget(m_recordBtn);

    m_recordAppendBtn = new QPushButton(tr("Append"), recordCluster);
    m_recordAppendBtn->setIcon(QIcon(":/icons/mic-plus.svg"));
    m_recordAppendBtn->setStyleSheet("background-color: #b91c1c; color: white; font-weight: 500; padding: 3px 7px; min-height: 24px;");
    m_recordAppendBtn->setToolTip(tr("Record additional sentences/paragraphs to existing session audio"));
    connect(m_recordAppendBtn, &QPushButton::clicked, this, &MainWindow::onRecordAppendToggle);
    recLay->addWidget(m_recordAppendBtn);

    m_retakeBtn = new QPushButton(tr("Retake"), recordCluster);
    m_retakeBtn->setIcon(QIcon(":/icons/rotate-ccw.svg"));
    m_retakeBtn->setToolTip(tr("Clear existing recording and prepare for a fresh take"));
    m_retakeBtn->setStyleSheet("padding: 3px 7px; min-height: 24px;");
    connect(m_retakeBtn, &QPushButton::clicked, this, &MainWindow::onRetake);
    recLay->addWidget(m_retakeBtn);

    m_pauseBtn = new QPushButton(recordCluster);
    m_pauseBtn->setIcon(QIcon(":/icons/pause.svg"));
    m_pauseBtn->setToolTip(tr("Pause / Resume recording"));
    m_pauseBtn->setEnabled(false);
    m_pauseBtn->setFixedSize(26, 26);
    connect(m_pauseBtn, &QPushButton::clicked, this, &MainWindow::onPauseToggle);
    recLay->addWidget(m_pauseBtn);

    m_recordingTimeLabel = new QLabel("00:00", recordCluster);
    m_recordingTimeLabel->setStyleSheet("font-family: monospace; font-size: 12px; font-weight: bold; color: #f87171;");
    recLay->addWidget(m_recordingTimeLabel);
    flowLayout->addWidget(recordCluster);

    // Cluster 3: Meter & Gain Boost
    auto *gainCluster = new QWidget(m_flowContainer);
    auto *gainLay = new QHBoxLayout(gainCluster);
    gainLay->setContentsMargins(0, 0, 0, 0);
    gainLay->setSpacing(4);

    m_levelMeter = new AudioLevelMeter(gainCluster);
    m_levelMeter->setFixedWidth(40);
    m_levelMeter->setFixedHeight(16);
    gainLay->addWidget(m_levelMeter);

    auto *gainLbl = new QLabel(tr("Gain:"), gainCluster);
    gainLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
    gainLay->addWidget(gainLbl);

    m_micGainSlider = new QSlider(Qt::Horizontal, gainCluster);
    m_micGainSlider->setRange(10, 40);
    m_micGainSlider->setValue(static_cast<int>(m_recorder->inputGain() * 10.0f));
    m_micGainSlider->setFixedWidth(40);
    m_micGainSlider->setToolTip(tr("Microphone input boost (1.0x - 4.0x)"));
    m_gainValueLabel = new QLabel(QString("%1x").arg(m_recorder->inputGain(), 0, 'f', 1), gainCluster);
    m_gainValueLabel->setStyleSheet("font-family: monospace; font-size: 11px; color: #a5b4fc; font-weight: 600;");
    connect(m_micGainSlider, &QSlider::valueChanged, this, [this](int val) {
        float gain = val / 10.0f;
        m_recorder->setInputGain(gain);
        m_gainValueLabel->setText(QString("%1x").arg(gain, 0, 'f', 1));
    });
    gainLay->addWidget(m_micGainSlider);
    gainLay->addWidget(m_gainValueLabel);
    flowLayout->addWidget(gainCluster);

    // Cluster 4: Whisper Model
    auto *modelCluster = new QWidget(m_flowContainer);
    auto *modelLay = new QHBoxLayout(modelCluster);
    modelLay->setContentsMargins(0, 0, 0, 0);
    modelLay->setSpacing(4);

    auto *modelLbl = new QLabel(tr("Model:"), modelCluster);
    modelLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
    modelLay->addWidget(modelLbl);

    m_modelCombo = new QComboBox(modelCluster);
    m_modelCombo->setMinimumWidth(110);
    m_modelCombo->setMaximumWidth(160);
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModelSelectionChanged);
    modelLay->addWidget(m_modelCombo);

    m_addModelBtn = new QPushButton(modelCluster);
    m_addModelBtn->setIcon(QIcon(":/icons/folder-plus.svg"));
    m_addModelBtn->setToolTip(tr("Import Local Whisper Model (.bin / .gguf)"));
    m_addModelBtn->setFixedSize(26, 26);
    connect(m_addModelBtn, &QPushButton::clicked, this, &MainWindow::onAddCustomModelClicked);
    modelLay->addWidget(m_addModelBtn);
    flowLayout->addWidget(modelCluster);

    // Cluster 5: Language & Transcribe
    auto *transCluster = new QWidget(m_flowContainer);
    auto *transLay = new QHBoxLayout(transCluster);
    transLay->setContentsMargins(0, 0, 0, 0);
    transLay->setSpacing(4);

    auto *langLbl = new QLabel(tr("Lang:"), transCluster);
    langLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
    transLay->addWidget(langLbl);

    m_languageCombo = new QComboBox(transCluster);
    m_languageCombo->addItem("German (de)", "de");
    m_languageCombo->addItem("English (en)", "en");
    m_languageCombo->addItem("Auto Detect", "auto");
    m_languageCombo->addItem("French (fr)", "fr");
    m_languageCombo->addItem("Spanish (es)", "es");
    m_languageCombo->addItem("Italian (it)", "it");
    m_languageCombo->setMinimumWidth(85);
    m_languageCombo->setMaximumWidth(120);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        QString lang = m_languageCombo->itemData(idx).toString();
        m_currentSession.language = lang;
        m_whisper->setLanguage(lang);
    });
    transLay->addWidget(m_languageCombo);

    m_transcribeBtn = new QPushButton(tr("Transcribe"), transCluster);
    m_transcribeBtn->setIcon(QIcon(":/icons/zap.svg"));
    m_transcribeBtn->setStyleSheet("background-color: #4f46e5; color: white; font-weight: 600; padding: 3px 12px; min-height: 24px;");
    connect(m_transcribeBtn, &QPushButton::clicked, this, &MainWindow::onTranscribeClicked);
    transLay->addWidget(m_transcribeBtn);
    flowLayout->addWidget(transCluster);

    // Cluster 6: Workspace Layout Switchers
    auto *layoutCluster = new QWidget(m_flowContainer);
    auto *layoutLay = new QHBoxLayout(layoutCluster);
    layoutLay->setContentsMargins(0, 0, 0, 0);
    layoutLay->setSpacing(3);

    auto *layoutLbl = new QLabel(tr("Layout:"), layoutCluster);
    layoutLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
    layoutLay->addWidget(layoutLbl);

    m_layoutSplitVBtn = new QPushButton(layoutCluster);
    m_layoutSplitVBtn->setIcon(QIcon(":/icons/layout-split-v.svg"));
    m_layoutSplitVBtn->setToolTip(tr("Stacked View (Notes on top, Transcript under it)"));
    m_layoutSplitVBtn->setFixedSize(26, 26);
    connect(m_layoutSplitVBtn, &QPushButton::clicked, this, [this]() {
        setWorkspaceLayout(WorkspaceLayout::StackedSplit);
    });
    layoutLay->addWidget(m_layoutSplitVBtn);

    m_layoutSplitHBtn = new QPushButton(layoutCluster);
    m_layoutSplitHBtn->setIcon(QIcon(":/icons/layout-split-h.svg"));
    m_layoutSplitHBtn->setToolTip(tr("Side-by-Side View (Notes left, Transcript right)"));
    m_layoutSplitHBtn->setFixedSize(26, 26);
    connect(m_layoutSplitHBtn, &QPushButton::clicked, this, [this]() {
        setWorkspaceLayout(WorkspaceLayout::SideSplit);
    });
    layoutLay->addWidget(m_layoutSplitHBtn);

    m_layoutTabsBtn = new QPushButton(layoutCluster);
    m_layoutTabsBtn->setIcon(QIcon(":/icons/layout-tabs.svg"));
    m_layoutTabsBtn->setToolTip(tr("Tabs View (Sentences, Notes, TTS Studio, Models)"));
    m_layoutTabsBtn->setFixedSize(26, 26);
    connect(m_layoutTabsBtn, &QPushButton::clicked, this, [this]() {
        setWorkspaceLayout(WorkspaceLayout::Tabbed);
    });
    layoutLay->addWidget(m_layoutTabsBtn);
    flowLayout->addWidget(layoutCluster);

    // Cluster 7: Models, Config & TTS Studio Tools
    auto *toolsCluster = new QWidget(m_flowContainer);
    auto *toolsLay = new QHBoxLayout(toolsCluster);
    toolsLay->setContentsMargins(0, 0, 0, 0);
    toolsLay->setSpacing(4);

    m_manageModelsBtn = new QPushButton(tr("Models"), toolsCluster);
    m_manageModelsBtn->setIcon(QIcon(":/icons/cpu.svg"));
    m_manageModelsBtn->setToolTip(tr("Open Whisper Models Manager (Download presets, import & delete models)"));
    m_manageModelsBtn->setStyleSheet("padding: 3px 8px; font-size: 11px; min-height: 24px;");
    connect(m_manageModelsBtn, &QPushButton::clicked, this, &MainWindow::onOpenModelManager);
    toolsLay->addWidget(m_manageModelsBtn);

    m_configBtn = new QPushButton(tr("Config"), toolsCluster);
    m_configBtn->setIcon(QIcon(":/icons/settings.svg"));
    m_configBtn->setToolTip(tr("Configure Whisper STT, Pocket TTS, threads & dock position"));
    m_configBtn->setStyleSheet("padding: 3px 8px; font-size: 11px; min-height: 24px;");
    connect(m_configBtn, &QPushButton::clicked, this, &MainWindow::onOpenConfigDialog);
    toolsLay->addWidget(m_configBtn);

    m_ttsStudioBtn = new QPushButton(tr("TTS"), toolsCluster);
    m_ttsStudioBtn->setIcon(QIcon(":/icons/volume.svg"));
    m_ttsStudioBtn->setToolTip(tr("Open Pocket TTS Speech Synthesis Studio"));
    m_ttsStudioBtn->setStyleSheet("padding: 3px 8px; font-size: 11px; min-height: 24px;");
    connect(m_ttsStudioBtn, &QPushButton::clicked, this, &MainWindow::onOpenTtsStudio);
    toolsLay->addWidget(m_ttsStudioBtn);
    flowLayout->addWidget(toolsCluster);

    mainLayout->addWidget(m_flowContainer);

    // Transcribe Progress Bar
    m_transcribeProgress = new QProgressBar(container);
    m_transcribeProgress->setRange(0, 100);
    m_transcribeProgress->setValue(0);
    m_transcribeProgress->setFixedHeight(3);
    m_transcribeProgress->setTextVisible(false);
    m_transcribeProgress->setStyleSheet(
        "QProgressBar { background: #1a1c23; border: none; border-radius: 1px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #818cf8, stop:1 #34d399); border-radius: 1px; }"
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

    topRow->addSpacing(10);

    // Takes control bar
    topRow->addWidget(new QLabel(tr("Takes:"), container));
    m_takeCombo = new QComboBox(container);
    m_takeCombo->setMinimumWidth(100);
    m_takeCombo->setMaximumWidth(160);
    m_takeCombo->setToolTip(tr("Select active take"));
    connect(m_takeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTakeSelected);
    topRow->addWidget(m_takeCombo);

    m_newTakeBtn = new QPushButton(tr("+ New Take"), container);
    m_newTakeBtn->setIcon(QIcon(":/icons/plus.svg"));
    m_newTakeBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600; padding: 2px 8px; font-size: 11px;");
    m_newTakeBtn->setToolTip(tr("Create a new take for this session"));
    connect(m_newTakeBtn, &QPushButton::clicked, this, &MainWindow::onNewTakeClicked);
    topRow->addWidget(m_newTakeBtn);

    m_deleteTakeBtn = new QPushButton(container);
    m_deleteTakeBtn->setIcon(QIcon(":/icons/trash-2.svg"));
    m_deleteTakeBtn->setToolTip(tr("Delete current take"));
    m_deleteTakeBtn->setFixedSize(26, 24);
    connect(m_deleteTakeBtn, &QPushButton::clicked, this, &MainWindow::onDeleteTakeClicked);
    topRow->addWidget(m_deleteTakeBtn);

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
    layout->setContentsMargins(4, 2, 4, 4);
    layout->setSpacing(4);

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
    ctrlRow->setSpacing(4);

    m_playBtn = new QPushButton(tr("Play"), container);
    m_playBtn->setIcon(QIcon(":/icons/play.svg"));
    m_playBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600; min-width: 65px; padding: 4px 8px;");
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayToggle);
    ctrlRow->addWidget(m_playBtn);

    m_stopBtn = new QPushButton(container);
    m_stopBtn->setIcon(QIcon(":/icons/stop.svg"));
    m_stopBtn->setToolTip(tr("Stop playback"));
    m_stopBtn->setFixedSize(28, 28);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    ctrlRow->addWidget(m_stopBtn);

    m_skipBackBtn = new QPushButton(tr("-5s"), container);
    m_skipBackBtn->setIcon(QIcon(":/icons/skip-back.svg"));
    m_skipBackBtn->setToolTip(tr("Skip backward 5s"));
    m_skipBackBtn->setStyleSheet("padding: 4px 6px;");
    connect(m_skipBackBtn, &QPushButton::clicked, this, [this]() { m_player->skipBackward(5000); });
    ctrlRow->addWidget(m_skipBackBtn);

    m_skipForwardBtn = new QPushButton(tr("+5s"), container);
    m_skipForwardBtn->setIcon(QIcon(":/icons/skip-forward.svg"));
    m_skipForwardBtn->setToolTip(tr("Skip forward 5s"));
    m_skipForwardBtn->setStyleSheet("padding: 4px 6px;");
    connect(m_skipForwardBtn, &QPushButton::clicked, this, [this]() { m_player->skipForward(5000); });
    ctrlRow->addWidget(m_skipForwardBtn);

    ctrlRow->addSpacing(6);

    m_replaySentenceBtn = new QPushButton(tr("Replay [R]"), container);
    m_replaySentenceBtn->setIcon(QIcon(":/icons/replay.svg"));
    m_replaySentenceBtn->setToolTip(tr("Replay currently selected sentence [R]"));
    m_replaySentenceBtn->setStyleSheet("background-color: #4338ca; color: white; font-weight: 600; padding: 4px 8px;");
    connect(m_replaySentenceBtn, &QPushButton::clicked, this, &MainWindow::onReplayCurrentSentence);
    ctrlRow->addWidget(m_replaySentenceBtn);

    m_loopSentenceBtn = new QPushButton(tr("Loop"), container);
    m_loopSentenceBtn->setIcon(QIcon(":/icons/loop.svg"));
    m_loopSentenceBtn->setToolTip(tr("Loop currently selected sentence continuously"));
    m_loopSentenceBtn->setCheckable(true);
    m_loopSentenceBtn->setStyleSheet("padding: 4px 8px;");
    connect(m_loopSentenceBtn, &QPushButton::toggled, this, &MainWindow::onLoopSentenceToggle);
    ctrlRow->addWidget(m_loopSentenceBtn);

    ctrlRow->addStretch();

    // Speed Selector
    auto *speedLbl = new QLabel(tr("Speed:"), container);
    speedLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
    ctrlRow->addWidget(speedLbl);

    m_speedCombo = new QComboBox(container);
    m_speedCombo->addItem("0.5x", 0.5);
    m_speedCombo->addItem("0.75x", 0.75);
    m_speedCombo->addItem("1.0x", 1.0);
    m_speedCombo->addItem("1.25x", 1.25);
    m_speedCombo->addItem("1.5x", 1.5);
    m_speedCombo->addItem("2.0x", 2.0);
    m_speedCombo->setCurrentIndex(2); // 1.0x
    m_speedCombo->setFixedWidth(68);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSpeedChanged);
    ctrlRow->addWidget(m_speedCombo);

    ctrlRow->addSpacing(6);

    // Volume & Mute
    m_muteBtn = new QPushButton(container);
    m_muteBtn->setIcon(QIcon(":/icons/volume.svg"));
    m_muteBtn->setFixedSize(28, 28);
    connect(m_muteBtn, &QPushButton::clicked, this, &MainWindow::onMuteToggle);
    ctrlRow->addWidget(m_muteBtn);

    m_volumeSlider = new QSlider(Qt::Horizontal, container);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(65);
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
    m_ttsInputEdit->setText("Guten Tag! Ich lerne heute Deutsch mit dem Orato Studio.");
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

    auto *topRow = new QHBoxLayout();
    auto *hdr = new QLabel(
        tr("<b>Whisper.cpp Models:</b> Download fine-tuned German or standard Whisper models, or import custom models."),
        container
    );
    hdr->setStyleSheet("color: #818cf8; font-size: 13px;");
    topRow->addWidget(hdr, 1);

    auto *importBtn = new QPushButton(tr("Import Model..."), container);
    importBtn->setIcon(QIcon(":/icons/folder-plus.svg"));
    connect(importBtn, &QPushButton::clicked, this, &MainWindow::onAddCustomModelClicked);
    topRow->addWidget(importBtn);

    auto *openDirBtn = new QPushButton(tr("Open Folder"), container);
    openDirBtn->setIcon(QIcon(":/icons/folder-open.svg"));
    connect(openDirBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_modelManager->modelsDirectory()));
    });
    topRow->addWidget(openDirBtn);

    layout->addLayout(topRow);

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
        auto *actionWidget = new QWidget(table);
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(2, 2, 2, 2);
        actionLayout->setSpacing(6);

        QString pid = p.id;
        if (installed) {
            auto *badge = new QLabel(tr("Installed ✓"), actionWidget);
            badge->setStyleSheet("color: #34d399; font-weight: 600; padding: 2px 6px;");
            actionLayout->addWidget(badge);

            auto *delBtn = new QPushButton(tr("Delete"), actionWidget);
            delBtn->setIcon(QIcon(":/icons/trash-2.svg"));
            delBtn->setStyleSheet("background-color: #991b1b; color: white; padding: 3px 8px; font-size: 11px;");
            connect(delBtn, &QPushButton::clicked, this, [this, pid]() {
                auto rep = QMessageBox::question(this, tr("Delete Model"),
                    tr("Are you sure you want to delete this model file?"),
                    QMessageBox::Yes | QMessageBox::No);
                if (rep == QMessageBox::Yes) {
                    m_modelManager->deletePresetModel(pid);
                    refreshModelList();
                }
            });
            actionLayout->addWidget(delBtn);
        } else {
            auto *actionBtn = new QPushButton(tr("Download"), actionWidget);
            actionBtn->setIcon(QIcon(":/icons/download.svg"));
            actionBtn->setStyleSheet("background-color: #4338ca; color: white; font-weight: 600; padding: 4px 10px;");
            connect(actionBtn, &QPushButton::clicked, this, [this, pid]() {
                onDownloadPresetRequested(pid);
            });
            actionLayout->addWidget(actionBtn);
        }
        table->setCellWidget(r, 3, actionWidget);
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
    if (m_sidebarWidget->isVisible()) {
        QList<int> sz = m_mainSplitter->sizes();
        int sideIdx = m_sidebarOnRight ? 1 : 0;
        if (sz.size() > sideIdx && sz[sideIdx] > 50) {
            m_savedSidebarWidth = sz[sideIdx];
        }
        m_sidebarWidget->setVisible(false);
    } else {
        m_sidebarWidget->setVisible(true);
        QList<int> sz = m_mainSplitter->sizes();
        int total = (sz.size() > 1) ? (sz[0] + sz[1]) : width();
        if (m_sidebarOnRight) {
            m_mainSplitter->setSizes({total - m_savedSidebarWidth, m_savedSidebarWidth});
        } else {
            m_mainSplitter->setSizes({m_savedSidebarWidth, total - m_savedSidebarWidth});
        }
    }
}

void MainWindow::onToggleSidebarDockSide() {
    m_sidebarOnRight = !m_sidebarOnRight;

    QList<int> currentSizes = m_mainSplitter->sizes();
    int sideWidth = (currentSizes.size() > 1) ? (m_sidebarOnRight ? currentSizes[0] : currentSizes[1]) : 280;
    int studioWidth = (currentSizes.size() > 1) ? (m_sidebarOnRight ? currentSizes[1] : currentSizes[0]) : 1000;
    if (sideWidth < 160) sideWidth = 280;

    m_sidebarWidget->setParent(nullptr);
    m_studioWidget->setParent(nullptr);

    if (m_sidebarOnRight) {
        m_mainSplitter->addWidget(m_studioWidget);
        m_mainSplitter->addWidget(m_sidebarWidget);
        m_mainSplitter->setSizes({studioWidth, sideWidth});
        m_dockSideBtn->setToolTip(tr("Dock Sessions Panel to Left"));
        statusBar()->showMessage(tr("Sessions panel docked to right side."), 3000);
    } else {
        m_mainSplitter->addWidget(m_sidebarWidget);
        m_mainSplitter->addWidget(m_studioWidget);
        m_mainSplitter->setSizes({sideWidth, studioWidth});
        m_dockSideBtn->setToolTip(tr("Dock Sessions Panel to Right"));
        statusBar()->showMessage(tr("Sessions panel docked to left side."), 3000);
    }
}

void MainWindow::onOpenModelManager() {
    ModelManagerDialog dlg(m_modelManager, m_whisper, this);
    dlg.exec();
    refreshModelList();
}

void MainWindow::onOpenConfigDialog() {
    ConfigDialog dlg(m_whisper, m_modelManager, m_tts, m_player, m_sidebarOnRight, this);
    connect(&dlg, &ConfigDialog::sidebarDockSideChanged, this, [this](bool onRight) {
        if (onRight != m_sidebarOnRight) {
            onToggleSidebarDockSide();
        }
    });
    connect(&dlg, &ConfigDialog::openModelManagerRequested, this, &MainWindow::onOpenModelManager);
    if (dlg.exec() == QDialog::Accepted) {
        refreshModelList();
        statusBar()->showMessage(tr("Configuration applied successfully."), 3000);
    }
}

void MainWindow::onOpenTtsStudio() {
    setWorkspaceLayout(WorkspaceLayout::Tabbed);
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabText(i).contains("TTS", Qt::CaseInsensitive)) {
            m_tabWidget->setCurrentIndex(i);
            break;
        }
    }
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
        m_recordBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: 600; padding: 4px 10px;");
        m_recordAppendBtn->setEnabled(true);
        m_pauseBtn->setEnabled(false);
    } else {
        QVariant data = m_inputDeviceCombo->currentData();
        QAudioDevice dev = data.value<QAudioDevice>();

        if (m_recorder->startRecording(dev, false)) {
            m_recordBtn->setText(tr("Stop"));
            m_recordBtn->setIcon(QIcon(":/icons/stop.svg"));
            m_recordBtn->setStyleSheet("background-color: #b91c1c; color: #fef08a; font-weight: 700; border: 2px solid #eab308; padding: 4px 10px;");
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
        m_recordAppendBtn->setStyleSheet("background-color: #b91c1c; color: white; font-weight: 500; padding: 4px 8px;");
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
            m_recordAppendBtn->setStyleSheet("background-color: #7f1d1d; color: #fef08a; font-weight: 700; border: 2px solid #f97316; padding: 4px 8px;");
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

    if (m_currentSession.takes.isEmpty()) {
        m_currentSession.addNewTake("Take 1");
    }
    auto *curTake = m_currentSession.currentTake();
    if (curTake) {
        curTake->durationMs = durationMs;
        curTake->segments = m_currentSession.segments;
        if (curTake->audioFileName.isEmpty()) {
            curTake->audioFileName = curTake->id + ".wav";
        }
        QString takePath = m_sessionManager->getTakeAudioPath(m_currentSession.id, curTake->audioFileName);
        AudioUtils::writeWavFile(takePath, m_currentAudioPcm, 16000, 1);
    }

    m_waveformWidget->setAudioData(m_currentAudioPcm, durationMs);
    m_player->setSource(savedPath);

    onSaveSession();
    updateTakesUi();
    statusBar()->showMessage(tr("Recording complete (%1). Ready to transcribe.").arg(formatTime(durationMs)));
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
        m_playBtn->setStyleSheet("background-color: #d97706; color: white; font-weight: 600; min-width: 65px; padding: 4px 8px;");
    } else {
        m_playBtn->setText(tr("Play"));
        m_playBtn->setIcon(QIcon(":/icons/play.svg"));
        m_playBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600; min-width: 65px; padding: 4px 8px;");
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

    auto *curTake = m_currentSession.currentTake();
    if (curTake) {
        curTake->segments = m_currentSession.segments;
        curTake->durationMs = m_currentSession.durationMs;
    }

    m_sessionManager->saveSession(m_currentSession, m_currentAudioPcm);
    updateTakesUi();
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
    if (m_topSessionTitleEdit) {
        m_topSessionTitleEdit->setText(m_currentSession.title);
    }
    if (m_sessionDateBadge) {
        m_sessionDateBadge->setText(m_currentSession.createdAt.toString("yyyy-MM-dd hh:mm"));
    }

    m_noteEditor->setMarkdownText(m_currentSession.noteMarkdown);
    updateTakesUi();

    auto *curTake = m_currentSession.currentTake();
    if (curTake) {
        m_sentenceListView->setSegments(curTake->segments);
        m_waveformWidget->setAudioData(m_currentAudioPcm, curTake->durationMs);
        m_waveformWidget->setSegments(curTake->segments);
        QString takePath = m_sessionManager->getTakeAudioPath(m_currentSession.id, curTake->audioFileName);
        m_player->setSource(QFile::exists(takePath) ? takePath : QString());
        m_recordingTimeLabel->setText(formatTime(curTake->durationMs));
    } else {
        m_sentenceListView->setSegments(m_currentSession.segments);
        m_waveformWidget->setAudioData(m_currentAudioPcm, m_currentSession.durationMs);
        m_waveformWidget->setSegments(m_currentSession.segments);
        QString audioPath = m_sessionManager->getAudioPath(m_currentSession.id);
        m_player->setSource(QFile::exists(audioPath) ? audioPath : QString());
        m_recordingTimeLabel->setText(formatTime(m_currentSession.durationMs));
    }
}

void MainWindow::updateTakesUi() {
    if (!m_takeCombo) return;
    m_takeCombo->blockSignals(true);
    m_takeCombo->clear();
    for (int i = 0; i < m_currentSession.takes.size(); ++i) {
        const auto &take = m_currentSession.takes[i];
        QString label = QString("%1 (%2)").arg(take.name, formatTime(take.durationMs));
        m_takeCombo->addItem(label, i);
    }
    if (m_currentSession.currentTakeIndex >= 0 && m_currentSession.currentTakeIndex < m_takeCombo->count()) {
        m_takeCombo->setCurrentIndex(m_currentSession.currentTakeIndex);
    }
    m_takeCombo->blockSignals(false);
    if (m_deleteTakeBtn) {
        m_deleteTakeBtn->setEnabled(m_currentSession.takes.size() > 1);
    }
    if (m_sessionTakeBadge) {
        int cur = m_currentSession.currentTakeIndex + 1;
        int tot = std::max(1, static_cast<int>(m_currentSession.takes.size()));
        m_sessionTakeBadge->setText(tr("Take %1 of %2").arg(cur).arg(tot));
    }
}

void MainWindow::onTakeSelected(int index) {
    if (index < 0 || index >= m_currentSession.takes.size()) return;
    if (index == m_currentSession.currentTakeIndex && !m_currentAudioPcm.empty()) return;

    m_currentSession.currentTakeIndex = index;
    auto *take = m_currentSession.currentTake();
    if (take) {
        QString takePath = m_sessionManager->getTakeAudioPath(m_currentSession.id, take->audioFileName);
        if (QFile::exists(takePath)) {
            uint32_t dur = 0;
            AudioUtils::loadWavToMono16k(takePath, m_currentAudioPcm, dur);
            m_currentSession.durationMs = dur;
            take->durationMs = dur;
            m_waveformWidget->setAudioData(m_currentAudioPcm, dur);
            m_player->setSource(takePath);
            m_recordingTimeLabel->setText(formatTime(dur));
        } else {
            m_currentAudioPcm.clear();
            m_currentSession.durationMs = 0;
            m_waveformWidget->setAudioData({}, 0);
            m_player->stop();
            m_player->setSource(QString());
            m_recordingTimeLabel->setText("00:00");
        }
        m_currentSession.segments = take->segments;
        m_sentenceListView->setSegments(take->segments);
        m_waveformWidget->setSegments(take->segments);
        statusBar()->showMessage(tr("Active take: %1").arg(take->name), 2000);
    }
}

void MainWindow::onNewTakeClicked() {
    auto &newTake = m_currentSession.addNewTake();
    m_currentAudioPcm.clear();
    m_currentSession.durationMs = 0;
    m_currentSession.segments.clear();

    m_waveformWidget->setAudioData({}, 0);
    m_waveformWidget->setSegments({});
    m_sentenceListView->setSegments({});
    m_player->stop();
    m_player->setSource(QString());
    m_recordingTimeLabel->setText("00:00");

    m_sessionManager->saveSession(m_currentSession);
    updateTakesUi();
    statusBar()->showMessage(tr("Created %1. Ready to record.").arg(newTake.name), 3000);
}

void MainWindow::onDeleteTakeClicked() {
    if (m_currentSession.takes.size() <= 1) return;
    auto *take = m_currentSession.currentTake();
    if (!take) return;

    auto reply = QMessageBox::question(
        this, tr("Delete Take"),
        tr("Are you sure you want to delete '%1'?").arg(take->name),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply != QMessageBox::Yes) return;

    QString takePath = m_sessionManager->getTakeAudioPath(m_currentSession.id, take->audioFileName);
    if (QFile::exists(takePath)) {
        QFile::remove(takePath);
    }

    m_currentSession.removeTake(m_currentSession.currentTakeIndex);
    m_sessionManager->saveSession(m_currentSession);
    updateTakesUi();

    auto *newActive = m_currentSession.currentTake();
    if (newActive) {
        QString path = m_sessionManager->getTakeAudioPath(m_currentSession.id, newActive->audioFileName);
        if (QFile::exists(path)) {
            uint32_t dur = 0;
            AudioUtils::loadWavToMono16k(path, m_currentAudioPcm, dur);
            m_currentSession.durationMs = dur;
            m_waveformWidget->setAudioData(m_currentAudioPcm, dur);
            m_player->setSource(path);
            m_recordingTimeLabel->setText(formatTime(dur));
        } else {
            m_currentAudioPcm.clear();
            m_currentSession.durationMs = 0;
            m_waveformWidget->setAudioData({}, 0);
            m_player->stop();
            m_player->setSource(QString());
            m_recordingTimeLabel->setText("00:00");
        }
        m_currentSession.segments = newActive->segments;
        m_sentenceListView->setSegments(newActive->segments);
        m_waveformWidget->setSegments(newActive->segments);
    }
    statusBar()->showMessage(tr("Take deleted."), 2000);
}

void MainWindow::updateSessionList() {
    m_sessionListWidget->blockSignals(true);
    m_sessionListWidget->clear();

    auto sessions = m_sessionManager->listSessions();
    for (const auto &s : sessions) {
        QString displayTitle = s.title.trimmed().isEmpty() ? tr("Untitled Session") : s.title;

        auto *item = new QListWidgetItem(m_sessionListWidget);
        item->setData(Qt::DisplayRole, displayTitle);
        item->setData(Qt::UserRole + 1, s.createdAt.toString("yyyy-MM-dd hh:mm"));
        item->setData(Qt::UserRole + 2, formatTime(s.durationMs));
        item->setData(Qt::UserRole + 3, s.id);
        item->setData(Qt::UserRole + 4, s.takes.size());

        item->setToolTip(QString("%1\nCreated: %2\nDuration: %3\nTakes: %4")
            .arg(displayTitle)
            .arg(s.createdAt.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(formatTime(s.durationMs))
            .arg(s.takes.size()));

        m_sessionListWidget->addItem(item);
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

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_sessionListWidget && event->type() == QEvent::Resize) {
        m_sessionListWidget->doItemsLayout();
    }
    if (watched == m_flowContainer && event->type() == QEvent::Resize) {
        auto *re = static_cast<QResizeEvent*>(event);
        if (re->oldSize().width() != re->size().width()) {
            m_flowContainer->updateGeometry();
            if (m_topBarWidget) {
                m_topBarWidget->updateGeometry();
                if (m_topBarWidget->parentWidget() && m_topBarWidget->parentWidget()->layout()) {
                    m_topBarWidget->parentWidget()->layout()->activate();
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_flowContainer) {
        m_flowContainer->updateGeometry();
    }
    if (m_topBarWidget) {
        m_topBarWidget->updateGeometry();
        if (m_topBarWidget->parentWidget() && m_topBarWidget->parentWidget()->layout()) {
            m_topBarWidget->parentWidget()->layout()->activate();
        }
    }
}
