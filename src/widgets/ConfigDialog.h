#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include "../whisper/WhisperEngine.h"
#include "../whisper/ModelManager.h"
#include "../tts/PocketTTSEngine.h"
#include "../audio/AudioPlayer.h"

class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigDialog(WhisperEngine *whisper,
                          ModelManager *modelManager,
                          PocketTTSEngine *tts,
                          AudioPlayer *player,
                          bool sidebarOnRight,
                          QWidget *parent = nullptr);
    ~ConfigDialog() override = default;

    bool isSidebarOnRight() const;

signals:
    void sidebarDockSideChanged(bool onRight);
    void openModelManagerRequested();

private slots:
    void onTestVoiceClicked();
    void onApplyClicked();

private:
    void setupUi();
    void setupWhisperTab();
    void setupTtsTab();
    void setupWorkspaceTab();

    WhisperEngine *m_whisper = nullptr;
    ModelManager *m_modelManager = nullptr;
    PocketTTSEngine *m_tts = nullptr;
    AudioPlayer *m_player = nullptr;
    bool m_sidebarOnRight = false;

    QTabWidget *m_tabWidget = nullptr;

    // Whisper Tab Widgets
    QComboBox *m_whisperModelCombo = nullptr;
    QComboBox *m_whisperLangCombo = nullptr;
    QSpinBox *m_whisperThreadsSpin = nullptr;
    QCheckBox *m_whisperTranslateCheck = nullptr;

    // Pocket TTS Tab Widgets
    QLabel *m_ttsEngineStatusLabel = nullptr;
    QComboBox *m_ttsVoiceCombo = nullptr;
    QLineEdit *m_ttsTestInput = nullptr;
    QPushButton *m_ttsTestBtn = nullptr;
    QLabel *m_ttsTestStatusLabel = nullptr;

    // Workspace Tab Widgets
    QRadioButton *m_dockLeftRadio = nullptr;
    QRadioButton *m_dockRightRadio = nullptr;
};
