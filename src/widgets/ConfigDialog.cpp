#include "ConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QFileInfo>
#include <thread>

ConfigDialog::ConfigDialog(WhisperEngine *whisper,
                           ModelManager *modelManager,
                           PocketTTSEngine *tts,
                           AudioPlayer *player,
                           bool sidebarOnRight,
                           QWidget *parent)
    : QDialog(parent)
    , m_whisper(whisper)
    , m_modelManager(modelManager)
    , m_tts(tts)
    , m_player(player)
    , m_sidebarOnRight(sidebarOnRight) {
    setWindowTitle(tr("Orato — Whisper & Pocket TTS Configuration"));
    resize(640, 520);
    setStyleSheet("QDialog { background-color: #0f1013; color: #f8fafc; }");

    setupUi();
}

bool ConfigDialog::isSidebarOnRight() const {
    return m_dockRightRadio ? m_dockRightRadio->isChecked() : m_sidebarOnRight;
}

void ConfigDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    auto *headerLabel = new QLabel(tr("Engine & Workspace Configuration"), this);
    headerLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #818cf8;");
    mainLayout->addWidget(headerLabel);

    m_tabWidget = new QTabWidget(this);
    setupWhisperTab();
    setupTtsTab();
    setupWorkspaceTab();
    mainLayout->addWidget(m_tabWidget, 1);

    // Bottom action row
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();

    auto *cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto *applyBtn = new QPushButton(tr("Apply & Save"), this);
    applyBtn->setStyleSheet("background-color: #4f46e5; color: white; font-weight: 600; padding: 6px 16px;");
    connect(applyBtn, &QPushButton::clicked, this, &ConfigDialog::onApplyClicked);
    btnRow->addWidget(applyBtn);

    mainLayout->addLayout(btnRow);
}

void ConfigDialog::setupWhisperTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    // Model Selector + Manage Button
    auto *modelRow = new QHBoxLayout();
    m_whisperModelCombo = new QComboBox(tab);
    m_whisperModelCombo->setMinimumWidth(260);

    auto models = m_modelManager->installedModels();
    for (const auto &m : models) {
        m_whisperModelCombo->addItem(m.name, m.filePath);
    }
    // Select current model if set
    int curIdx = m_whisperModelCombo->findData(m_whisper->modelPath());
    if (curIdx >= 0) m_whisperModelCombo->setCurrentIndex(curIdx);
    modelRow->addWidget(m_whisperModelCombo, 1);

    auto *manageBtn = new QPushButton(tr("Manage Models..."), tab);
    manageBtn->setIcon(QIcon(":/icons/cpu.svg"));
    connect(manageBtn, &QPushButton::clicked, this, [this]() {
        emit openModelManagerRequested();
    });
    modelRow->addWidget(manageBtn);
    formLayout->addRow(new QLabel(tr("Active Model:"), tab), modelRow);

    // Default Language
    m_whisperLangCombo = new QComboBox(tab);
    m_whisperLangCombo->addItem("German (de)", "de");
    m_whisperLangCombo->addItem("English (en)", "en");
    m_whisperLangCombo->addItem("Auto Detect", "auto");
    m_whisperLangCombo->addItem("French (fr)", "fr");
    m_whisperLangCombo->addItem("Spanish (es)", "es");
    m_whisperLangCombo->addItem("Italian (it)", "it");
    int langIdx = m_whisperLangCombo->findData(m_whisper->language());
    if (langIdx >= 0) m_whisperLangCombo->setCurrentIndex(langIdx);
    formLayout->addRow(new QLabel(tr("Transcription Language:"), tab), m_whisperLangCombo);

    // CPU Threads
    auto *threadRow = new QHBoxLayout();
    m_whisperThreadsSpin = new QSpinBox(tab);
    m_whisperThreadsSpin->setRange(1, 64);
    int hw = std::thread::hardware_concurrency();
    m_whisperThreadsSpin->setValue(m_whisper->threads() > 0 ? m_whisper->threads() : (hw > 2 ? hw / 2 : 2));
    threadRow->addWidget(m_whisperThreadsSpin);
    auto *threadHint = new QLabel(tr("(Detected %1 hardware cores)").arg(hw), tab);
    threadHint->setStyleSheet("color: #94a3b8; font-size: 11px;");
    threadRow->addWidget(threadHint);
    threadRow->addStretch();
    formLayout->addRow(new QLabel(tr("CPU Threads:"), tab), threadRow);

    // Translation toggle
    m_whisperTranslateCheck = new QCheckBox(tr("Translate to English during transcription"), tab);
    m_whisperTranslateCheck->setChecked(m_whisper->translate());
    formLayout->addRow(new QLabel(tr("Task:"), tab), m_whisperTranslateCheck);

    layout->addLayout(formLayout);

    auto *infoBox = new QLabel(
        tr("<b>Tip:</b> For German pronunciation training, the fine-tuned "
           "<code>primeline/whisper-tiny-german (Q8_0)</code> provides fast inference and "
           "high German phoneme accuracy."),
        tab
    );
    infoBox->setStyleSheet("background-color: #1a1c23; border: 1px solid #272a34; padding: 10px; border-radius: 6px; font-size: 12px;");
    infoBox->setWordWrap(true);
    layout->addWidget(infoBox);

    layout->addStretch();
    m_tabWidget->addTab(tab, QIcon(":/icons/cpu.svg"), tr("Whisper STT"));
}

void ConfigDialog::setupTtsTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    // Engine Status
    auto *statusGroup = new QGroupBox(tr("Pocket TTS Engine Status"), tab);
    auto *statusLayout = new QVBoxLayout(statusGroup);
    m_ttsEngineStatusLabel = new QLabel(statusGroup);
    if (m_tts->isAvailable()) {
        m_ttsEngineStatusLabel->setText(
            QString("<span style='color:#34d399; font-weight:bold;'>✓ Ready & Available</span><br>"
                    "<span style='color:#94a3b8; font-size:11px;'>Engine path: %1</span>")
                .arg(m_tts->enginePath())
        );
    } else {
        m_ttsEngineStatusLabel->setText(
            tr("<span style='color:#f87171; font-weight:bold;'>✗ Pocket TTS not found in default paths</span><br>"
               "<span style='color:#94a3b8; font-size:11px;'>Ensure <code>./pocket-tts/.venv</code> or <code>uv</code> is installed.</span>")
        );
    }
    statusLayout->addWidget(m_ttsEngineStatusLabel);
    layout->addWidget(statusGroup);

    // Voice Selection
    auto *voiceGroup = new QGroupBox(tr("Default Voice & Pronunciation"), tab);
    auto *voiceForm = new QFormLayout(voiceGroup);

    m_ttsVoiceCombo = new QComboBox(voiceGroup);
    for (const auto &v : m_tts->availableVoices()) {
        m_ttsVoiceCombo->addItem(QString("%1 (%2)").arg(v.displayName, v.language.toUpper()), v.id);
    }
    voiceForm->addRow(new QLabel(tr("Preferred Voice:"), voiceGroup), m_ttsVoiceCombo);
    layout->addWidget(voiceGroup);

    // Live Voice Test
    auto *testGroup = new QGroupBox(tr("Test Pocket TTS Voice"), tab);
    auto *testLayout = new QVBoxLayout(testGroup);

    auto *testRow = new QHBoxLayout();
    m_ttsTestInput = new QLineEdit(testGroup);
    m_ttsTestInput->setText("Guten Tag! Willkommen bei Orato.");
    testRow->addWidget(m_ttsTestInput, 1);

    m_ttsTestBtn = new QPushButton(tr("Test Voice"), testGroup);
    m_ttsTestBtn->setIcon(QIcon(":/icons/volume.svg"));
    m_ttsTestBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: 600; padding: 6px 14px;");
    connect(m_ttsTestBtn, &QPushButton::clicked, this, &ConfigDialog::onTestVoiceClicked);
    testRow->addWidget(m_ttsTestBtn);
    testLayout->addLayout(testRow);

    m_ttsTestStatusLabel = new QLabel(testGroup);
    m_ttsTestStatusLabel->setStyleSheet("color: #818cf8; font-size: 11px;");
    testLayout->addWidget(m_ttsTestStatusLabel);

    layout->addWidget(testGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, QIcon(":/icons/volume.svg"), tr("Pocket TTS"));
}

void ConfigDialog::setupWorkspaceTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    auto *panelGroup = new QGroupBox(tr("Sessions Panel Placement"), tab);
    auto *panelLayout = new QVBoxLayout(panelGroup);

    m_dockLeftRadio = new QRadioButton(tr("Dock Sessions Panel on Left Side (Standard)"), panelGroup);
    m_dockRightRadio = new QRadioButton(tr("Dock Sessions Panel on Right Side"), panelGroup);

    if (m_sidebarOnRight) {
        m_dockRightRadio->setChecked(true);
    } else {
        m_dockLeftRadio->setChecked(true);
    }

    panelLayout->addWidget(m_dockLeftRadio);
    panelLayout->addWidget(m_dockRightRadio);
    layout->addWidget(panelGroup);

    auto *descLabel = new QLabel(
        tr("<b>Tip:</b> You can also switch the Sessions Panel between Left and Right at any time "
           "using the Dock button in the Sessions panel header, or resize it freely by dragging the divider."),
        tab
    );
    descLabel->setStyleSheet("background-color: #1a1c23; border: 1px solid #272a34; padding: 10px; border-radius: 6px; font-size: 12px;");
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    layout->addStretch();
    m_tabWidget->addTab(tab, QIcon(":/icons/dock.svg"), tr("Workspace & Docking"));
}

void ConfigDialog::onTestVoiceClicked() {
    QString text = m_ttsTestInput->text().trimmed();
    if (text.isEmpty()) return;

    QString voice = m_ttsVoiceCombo->currentData().toString();
    QString outPath = QString("/tmp/orato_test_voice_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());

    m_ttsTestBtn->setEnabled(false);
    m_ttsTestStatusLabel->setText(tr("Synthesizing voice..."));

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(m_tts, &PocketTTSEngine::generationCompleted, this, [this, outPath, conn](const QString &path) {
        m_ttsTestBtn->setEnabled(true);
        m_ttsTestStatusLabel->setText(tr("Playing test speech..."));
        m_player->setSource(path);
        m_player->play();
        QObject::disconnect(*conn);
    });

    auto errConn = std::make_shared<QMetaObject::Connection>();
    *errConn = connect(m_tts, &PocketTTSEngine::generationFailed, this, [this, errConn](const QString &err) {
        m_ttsTestBtn->setEnabled(true);
        m_ttsTestStatusLabel->setText(tr("Test failed: %1").arg(err));
        QObject::disconnect(*errConn);
    });

    m_tts->generateSpeech(text, outPath, "german", voice);
}

void ConfigDialog::onApplyClicked() {
    // Apply Whisper settings
    QString selectedModel = m_whisperModelCombo->currentData().toString();
    if (!selectedModel.isEmpty()) {
        m_whisper->setModelPath(selectedModel);
    }
    m_whisper->setLanguage(m_whisperLangCombo->currentData().toString());
    m_whisper->setThreads(m_whisperThreadsSpin->value());
    m_whisper->setTranslate(m_whisperTranslateCheck->isChecked());

    // Apply Dock side
    bool newOnRight = m_dockRightRadio->isChecked();
    if (newOnRight != m_sidebarOnRight) {
        m_sidebarOnRight = newOnRight;
        emit sidebarDockSideChanged(m_sidebarOnRight);
    }

    accept();
}
