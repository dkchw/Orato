#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QTextEdit>
#include "../whisper/ModelManager.h"
#include "../whisper/WhisperEngine.h"

class ModelManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ModelManagerDialog(ModelManager *modelManager, WhisperEngine *whisperEngine, QWidget *parent = nullptr);
    ~ModelManagerDialog() override = default;

public slots:
    void refreshAll();

private slots:
    void onImportModelClicked();
    void onOpenFolderClicked();
    void onDeleteInstalledClicked(const QString &path);
    void onDeletePresetClicked(const QString &presetId);
    void onDownloadPresetClicked(const QString &presetId);
    void onConvertHfClicked();

private:
    void setupUi();
    void updateInstalledTable();
    void updatePresetsTable();

    ModelManager *m_modelManager = nullptr;
    WhisperEngine *m_whisper = nullptr;

    QTableWidget *m_installedTable = nullptr;
    QTableWidget *m_presetsTable = nullptr;

    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;

    QLineEdit *m_hfInputEdit = nullptr;
    QPushButton *m_convertBtn = nullptr;
    QTextEdit *m_convertLog = nullptr;
};
