#include "ModelManagerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QGroupBox>
#include <QIcon>

ModelManagerDialog::ModelManagerDialog(ModelManager *modelManager, WhisperEngine *whisperEngine, QWidget *parent)
    : QDialog(parent)
    , m_modelManager(modelManager)
    , m_whisper(whisperEngine) {
    setWindowTitle(tr("Whisper Models Manager"));
    resize(860, 680);
    setStyleSheet("QDialog { background-color: #0f1013; color: #f8fafc; }");

    setupUi();

    connect(m_modelManager, &ModelManager::modelsListChanged, this, &ModelManagerDialog::refreshAll);
    connect(m_modelManager, &ModelManager::downloadStarted, this, [this](const QString &name) {
        m_statusLabel->setText(tr("Downloading %1...").arg(name));
        m_progressBar->setValue(0);
        m_progressBar->show();
    });
    connect(m_modelManager, &ModelManager::downloadProgress, this, [this](qint64, qint64, int pct) {
        m_progressBar->setValue(pct);
    });
    connect(m_modelManager, &ModelManager::downloadCompleted, this, [this](const QString &path) {
        m_statusLabel->setText(tr("Successfully installed: %1").arg(path));
        m_progressBar->hide();
        refreshAll();
    });
    connect(m_modelManager, &ModelManager::downloadFailed, this, [this](const QString &err) {
        m_statusLabel->setText(tr("Download error: %1").arg(err));
        m_progressBar->hide();
    });

    connect(m_modelManager, &ModelManager::conversionProgress, this, [this](const QString &txt) {
        m_convertLog->append(txt);
    });
    connect(m_modelManager, &ModelManager::conversionFinished, this, [this](bool ok, const QString &res) {
        m_convertBtn->setEnabled(true);
        if (ok) {
            m_statusLabel->setText(tr("Conversion completed successfully!"));
            refreshAll();
        } else {
            m_statusLabel->setText(tr("Conversion failed: %1").arg(res));
        }
    });

    refreshAll();
}

void ModelManagerDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // Top action bar
    auto *topBar = new QHBoxLayout();
    auto *titleLabel = new QLabel(tr("Whisper.cpp Models Manager"), this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #818cf8;");
    topBar->addWidget(titleLabel);
    topBar->addStretch();

    auto *importBtn = new QPushButton(tr("Import Local Model..."), this);
    importBtn->setIcon(QIcon(":/icons/folder-plus.svg"));
    importBtn->setStyleSheet("background-color: #059669; color: white; font-weight: 600; padding: 6px 12px;");
    connect(importBtn, &QPushButton::clicked, this, &ModelManagerDialog::onImportModelClicked);
    topBar->addWidget(importBtn);

    auto *openDirBtn = new QPushButton(tr("Open Models Folder"), this);
    openDirBtn->setIcon(QIcon(":/icons/folder-open.svg"));
    connect(openDirBtn, &QPushButton::clicked, this, &ModelManagerDialog::onOpenFolderClicked);
    topBar->addWidget(openDirBtn);

    auto *refreshBtn = new QPushButton(tr("Refresh"), this);
    refreshBtn->setIcon(QIcon(":/icons/refresh-cw.svg"));
    connect(refreshBtn, &QPushButton::clicked, this, &ModelManagerDialog::refreshAll);
    topBar->addWidget(refreshBtn);

    mainLayout->addLayout(topBar);

    // Section 1: Installed Models
    auto *installedGroup = new QGroupBox(tr("Installed Whisper Models"), this);
    auto *installedLayout = new QVBoxLayout(installedGroup);

    m_installedTable = new QTableWidget(installedGroup);
    m_installedTable->setColumnCount(4);
    m_installedTable->setHorizontalHeaderLabels({tr("Model Name"), tr("Size"), tr("Type"), tr("Actions")});
    m_installedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_installedTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_installedTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_installedTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_installedTable->setStyleSheet("QTableWidget { background-color: #14161c; border: 1px solid #272a34; }");
    installedLayout->addWidget(m_installedTable);

    mainLayout->addWidget(installedGroup, 1);

    // Section 2: Preset Models Catalog
    auto *presetsGroup = new QGroupBox(tr("Available Preset Models (1-Click Download)"), this);
    auto *presetsLayout = new QVBoxLayout(presetsGroup);

    m_presetsTable = new QTableWidget(presetsGroup);
    m_presetsTable->setColumnCount(5);
    m_presetsTable->setHorizontalHeaderLabels({tr("Model"), tr("Language"), tr("Size"), tr("Status"), tr("Action")});
    m_presetsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_presetsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_presetsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_presetsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_presetsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_presetsTable->setStyleSheet("QTableWidget { background-color: #14161c; border: 1px solid #272a34; }");
    presetsLayout->addWidget(m_presetsTable);

    mainLayout->addWidget(presetsGroup, 1);

    // Download progress row
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #818cf8; font-size: 12px;");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);

    // Section 3: Hugging Face Converter
    auto *hfGroup = new QGroupBox(tr("Convert HuggingFace Model"), this);
    auto *hfLayout = new QVBoxLayout(hfGroup);
    auto *hfRow = new QHBoxLayout();
    m_hfInputEdit = new QLineEdit(hfGroup);
    m_hfInputEdit->setPlaceholderText(tr("e.g. primeline/whisper-tiny-german or full HuggingFace URL"));
    hfRow->addWidget(m_hfInputEdit);

    m_convertBtn = new QPushButton(tr("Convert & Import"), hfGroup);
    m_convertBtn->setIcon(QIcon(":/icons/zap.svg"));
    m_convertBtn->setStyleSheet("background-color: #4338ca; color: white; font-weight: 600;");
    connect(m_convertBtn, &QPushButton::clicked, this, &ModelManagerDialog::onConvertHfClicked);
    hfRow->addWidget(m_convertBtn);
    hfLayout->addLayout(hfRow);

    m_convertLog = new QTextEdit(hfGroup);
    m_convertLog->setMaximumHeight(70);
    m_convertLog->setReadOnly(true);
    m_convertLog->setStyleSheet("background-color: #0b0c0f; font-family: monospace; font-size: 11px;");
    hfLayout->addWidget(m_convertLog);

    mainLayout->addWidget(hfGroup);

    // Close button
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();
    auto *closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomRow->addWidget(closeBtn);
    mainLayout->addLayout(bottomRow);
}

void ModelManagerDialog::refreshAll() {
    m_modelManager->refreshInstalledModels();
    updateInstalledTable();
    updatePresetsTable();
}

void ModelManagerDialog::updateInstalledTable() {
    auto models = m_modelManager->installedModels();
    m_installedTable->setRowCount(models.size());

    for (int r = 0; r < models.size(); ++r) {
        const auto &m = models[r];
        m_installedTable->setItem(r, 0, new QTableWidgetItem(m.name));
        m_installedTable->setItem(r, 1, new QTableWidgetItem(QString("%1 MB").arg(m.sizeBytes / (1024 * 1024))));
        m_installedTable->setItem(r, 2, new QTableWidgetItem(m.isCustom ? tr("Custom") : tr("Preset")));

        auto *actionWidget = new QWidget(m_installedTable);
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(2, 2, 2, 2);
        actionLayout->setSpacing(6);

        // "Use Now" button
        auto *useBtn = new QPushButton(tr("Use Now"), actionWidget);
        useBtn->setStyleSheet("background-color: #2563eb; color: white; padding: 4px 8px;");
        QString path = m.filePath;
        connect(useBtn, &QPushButton::clicked, this, [this, path]() {
            m_whisper->setModelPath(path);
            m_statusLabel->setText(tr("Active Whisper model set to: %1").arg(path));
        });
        actionLayout->addWidget(useBtn);

        // Delete button
        auto *delBtn = new QPushButton(actionWidget);
        delBtn->setIcon(QIcon(":/icons/trash-2.svg"));
        delBtn->setToolTip(tr("Delete Model File"));
        delBtn->setStyleSheet("background-color: #991b1b; color: white; padding: 4px;");
        delBtn->setFixedSize(26, 26);
        connect(delBtn, &QPushButton::clicked, this, [this, path]() {
            onDeleteInstalledClicked(path);
        });
        actionLayout->addWidget(delBtn);

        m_installedTable->setCellWidget(r, 3, actionWidget);
    }
}

void ModelManagerDialog::updatePresetsTable() {
    auto presets = m_modelManager->presetModels();
    m_presetsTable->setRowCount(presets.size());

    for (int r = 0; r < presets.size(); ++r) {
        const auto &p = presets[r];
        m_presetsTable->setItem(r, 0, new QTableWidgetItem(p.name));
        m_presetsTable->setItem(r, 1, new QTableWidgetItem(p.language.toUpper()));
        m_presetsTable->setItem(r, 2, new QTableWidgetItem(QString("%1 MB").arg(p.approxSizeMb)));

        bool installed = m_modelManager->isModelInstalled(p.id);
        m_presetsTable->setItem(r, 3, new QTableWidgetItem(installed ? tr("Installed ✓") : tr("Available")));

        auto *btnContainer = new QWidget(m_presetsTable);
        auto *btnLayout = new QHBoxLayout(btnContainer);
        btnLayout->setContentsMargins(2, 2, 2, 2);
        btnLayout->setSpacing(6);

        QString pid = p.id;
        if (installed) {
            auto *delBtn = new QPushButton(tr("Delete"), btnContainer);
            delBtn->setIcon(QIcon(":/icons/trash-2.svg"));
            delBtn->setStyleSheet("background-color: #991b1b; color: white; padding: 4px 8px;");
            connect(delBtn, &QPushButton::clicked, this, [this, pid]() {
                onDeletePresetClicked(pid);
            });
            btnLayout->addWidget(delBtn);
        } else {
            auto *dlBtn = new QPushButton(tr("Download"), btnContainer);
            dlBtn->setIcon(QIcon(":/icons/download.svg"));
            dlBtn->setStyleSheet("background-color: #4338ca; color: white; padding: 4px 10px; font-weight: 600;");
            connect(dlBtn, &QPushButton::clicked, this, [this, pid]() {
                onDownloadPresetClicked(pid);
            });
            btnLayout->addWidget(dlBtn);
        }

        m_presetsTable->setCellWidget(r, 4, btnContainer);
    }
}

void ModelManagerDialog::onImportModelClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select Whisper GGML Model File"),
        QDir::homePath(),
        tr("Whisper Models (*.bin *.gguf);;All Files (*)")
    );

    if (!filePath.isEmpty()) {
        m_modelManager->addCustomModel(filePath);
        m_statusLabel->setText(tr("Imported model: %1").arg(filePath));
        refreshAll();
    }
}

void ModelManagerDialog::onOpenFolderClicked() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_modelManager->modelsDirectory()));
}

void ModelManagerDialog::onDeleteInstalledClicked(const QString &path) {
    auto reply = QMessageBox::question(
        this, tr("Confirm Delete"),
        tr("Are you sure you want to delete this model file?\n\n%1").arg(path),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (m_modelManager->removeModel(path)) {
            m_statusLabel->setText(tr("Model removed successfully."));
            refreshAll();
        } else {
            m_statusLabel->setText(tr("Failed to delete model."));
        }
    }
}

void ModelManagerDialog::onDeletePresetClicked(const QString &presetId) {
    auto reply = QMessageBox::question(
        this, tr("Confirm Delete"),
        tr("Are you sure you want to delete this model preset?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (m_modelManager->deletePresetModel(presetId)) {
            m_statusLabel->setText(tr("Model deleted successfully."));
            refreshAll();
        } else {
            m_statusLabel->setText(tr("Failed to delete model."));
        }
    }
}

void ModelManagerDialog::onDownloadPresetClicked(const QString &presetId) {
    m_modelManager->downloadPreset(presetId);
}

void ModelManagerDialog::onConvertHfClicked() {
    QString id = m_hfInputEdit->text().trimmed();
    if (id.isEmpty()) return;

    m_convertBtn->setEnabled(false);
    m_convertLog->clear();
    m_statusLabel->setText(tr("Starting conversion of %1...").arg(id));
    m_modelManager->convertHuggingFaceModel(id);
}
