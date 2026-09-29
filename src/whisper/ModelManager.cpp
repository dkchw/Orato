#include "ModelManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

ModelManager::ModelManager(QObject *parent)
    : QObject(parent) {
    m_netManager = new QNetworkAccessManager(this);
    initPresets();

    // Ensure models directory exists
    QDir().mkpath(modelsDirectory());

    // If we previously downloaded in /tmp/whisper_models, copy over
    QString tmpModel = "/tmp/whisper_models/ggml-tiny-german-q8_0.bin";
    QString destModel = modelsDirectory() + "/ggml-tiny-german-q8_0.bin";
    if (QFile::exists(tmpModel) && !QFile::exists(destModel)) {
        QFile::copy(tmpModel, destModel);
    }

    refreshInstalledModels();
}

ModelManager::~ModelManager() {
    cancelDownload();
}

QString ModelManager::modelsDirectory() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.local/share/recorder";
    }
    return dataDir + "/models";
}

void ModelManager::initPresets() {
    m_presets.clear();

    m_presets.append({
        "tiny-german-q8",
        "Whisper Tiny German (primeline, Q8_0 - Recommended)",
        "https://huggingface.co/Pomni/whisper-tiny-german-ggml-allquants/resolve/main/ggml-tiny-german-q8_0.bin",
        "ggml-tiny-german-q8_0.bin",
        "Fine-tuned primeline/whisper-tiny-german quantized to Q8_0 (fast, high accuracy)",
        "de",
        43
    });

    m_presets.append({
        "tiny-german-f16",
        "Whisper Tiny German (primeline, F16)",
        "https://huggingface.co/Pomni/whisper-tiny-german-ggml-allquants/resolve/main/ggml-tiny-german-f16.bin",
        "ggml-tiny-german-f16.bin",
        "Fine-tuned primeline/whisper-tiny-german uncompressed FP16",
        "de",
        77
    });

    m_presets.append({
        "tiny.en",
        "Whisper Tiny English",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin",
        "ggml-tiny.en.bin",
        "Fastest English model from OpenAI",
        "en",
        75
    });

    m_presets.append({
        "tiny",
        "Whisper Tiny Multilingual",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin",
        "ggml-tiny.bin",
        "Fastest multilingual model from OpenAI",
        "multilingual",
        75
    });

    m_presets.append({
        "base",
        "Whisper Base Multilingual",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin",
        "ggml-base.bin",
        "Balanced speed and accuracy for multilingual speech",
        "multilingual",
        142
    });

    m_presets.append({
        "small",
        "Whisper Small Multilingual",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin",
        "ggml-small.bin",
        "High accuracy multilingual model",
        "multilingual",
        466
    });
}

QList<ModelPreset> ModelManager::presetModels() const {
    return m_presets;
}

QList<InstalledModel> ModelManager::installedModels() const {
    return m_installed;
}

bool ModelManager::isModelInstalled(const QString &presetId) const {
    for (const auto &preset : m_presets) {
        if (preset.id == presetId) {
            QString path = modelsDirectory() + "/" + preset.fileName;
            return QFile::exists(path);
        }
    }
    return false;
}

QString ModelManager::getModelPath(const QString &presetId) const {
    for (const auto &preset : m_presets) {
        if (preset.id == presetId) {
            QString path = modelsDirectory() + "/" + preset.fileName;
            if (QFile::exists(path)) return path;
        }
    }
    return QString();
}

void ModelManager::refreshInstalledModels() {
    m_installed.clear();
    QDir dir(modelsDirectory());
    QStringList filters;
    filters << "*.bin" << "*.gguf";

    QFileInfoList entries = dir.entryInfoList(filters, QDir::Files, QDir::Time);
    for (const auto &info : entries) {
        InstalledModel model;
        model.name = info.fileName();
        model.filePath = info.absoluteFilePath();
        model.sizeBytes = info.size();

        bool isPreset = false;
        for (const auto &preset : m_presets) {
            if (preset.fileName == info.fileName()) {
                model.name = preset.name;
                isPreset = true;
                break;
            }
        }
        model.isCustom = !isPreset;
        m_installed.append(model);
    }

    emit modelsListChanged();
}

void ModelManager::downloadPreset(const QString &presetId) {
    if (m_currentReply) {
        emit downloadFailed(tr("Another download is already in progress."));
        return;
    }

    const ModelPreset *target = nullptr;
    for (const auto &p : m_presets) {
        if (p.id == presetId) {
            target = &p;
            break;
        }
    }

    if (!target) {
        emit downloadFailed(tr("Unknown preset: %1").arg(presetId));
        return;
    }

    m_downloadingModelName = target->name;
    m_downloadingDestPath = modelsDirectory() + "/" + target->fileName;
    QString partPath = m_downloadingDestPath + ".part";

    m_outputFile = new QFile(partPath, this);
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        emit downloadFailed(tr("Failed to open file for writing: %1").arg(partPath));
        delete m_outputFile;
        m_outputFile = nullptr;
        return;
    }

    QNetworkRequest request(QUrl(target->url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_currentReply = m_netManager->get(request);

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &ModelManager::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::readyRead, this, &ModelManager::onDownloadReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &ModelManager::onDownloadFinished);

    emit downloadStarted(m_downloadingModelName);
}

void ModelManager::cancelDownload() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_outputFile) {
        m_outputFile->close();
        m_outputFile->remove();
        delete m_outputFile;
        m_outputFile = nullptr;
    }
}

void ModelManager::onDownloadProgress(qint64 received, qint64 total) {
    int percent = (total > 0) ? static_cast<int>((received * 100) / total) : 0;
    emit downloadProgress(received, total, percent);
}

void ModelManager::onDownloadReadyRead() {
    if (m_currentReply && m_outputFile) {
        m_outputFile->write(m_currentReply->readAll());
    }
}

void ModelManager::onDownloadFinished() {
    if (!m_currentReply) return;

    if (m_currentReply->error() != QNetworkReply::NoError) {
        QString errStr = m_currentReply->errorString();
        cancelDownload();
        emit downloadFailed(errStr);
        return;
    }

    m_outputFile->close();
    QString partPath = m_outputFile->fileName();
    delete m_outputFile;
    m_outputFile = nullptr;

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    // Rename .part to final destination
    QFile::remove(m_downloadingDestPath);
    if (!QFile::rename(partPath, m_downloadingDestPath)) {
        emit downloadFailed(tr("Failed to finalize downloaded file."));
        return;
    }

    refreshInstalledModels();
    emit downloadCompleted(m_downloadingDestPath);
}

void ModelManager::addCustomModel(const QString &filePath) {
    if (!QFile::exists(filePath)) return;

    QFileInfo info(filePath);
    QString dest = modelsDirectory() + "/" + info.fileName();
    if (info.absoluteFilePath() != dest) {
        QFile::copy(filePath, dest);
    }
    refreshInstalledModels();
}

void ModelManager::convertHuggingFaceModel(const QString &hfModelIdOrUrl) {
    QString modelId = hfModelIdOrUrl;
    if (modelId.startsWith("https://huggingface.co/")) {
        modelId = modelId.mid(QString("https://huggingface.co/").length());
    }

    QString scriptPath = QCoreApplication::applicationDirPath() + "/../scripts/convert_huggingface_whisper.py";
    if (!QFile::exists(scriptPath)) {
        scriptPath = QDir::currentPath() + "/scripts/convert_huggingface_whisper.py";
    }

    if (!QFile::exists(scriptPath)) {
        emit conversionFinished(false, tr("Conversion script not found: %1").arg(scriptPath));
        return;
    }

    if (m_convertProcess) {
        m_convertProcess->kill();
        m_convertProcess->deleteLater();
    }

    m_convertProcess = new QProcess(this);
    connect(m_convertProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        emit conversionProgress(QString::fromUtf8(m_convertProcess->readAllStandardOutput()));
    });
    connect(m_convertProcess, &QProcess::readyReadStandardError, this, [this]() {
        emit conversionProgress(QString::fromUtf8(m_convertProcess->readAllStandardError()));
    });

    connect(m_convertProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, modelId](int exitCode, QProcess::ExitStatus) {
        bool ok = (exitCode == 0);
        refreshInstalledModels();
        emit conversionFinished(ok, ok ? modelsDirectory() : tr("Process exited with code %1").arg(exitCode));
        m_convertProcess->deleteLater();
        m_convertProcess = nullptr;
    });

    QStringList args;
    args << scriptPath << modelId << modelsDirectory();
    m_convertProcess->start("bash", args);
}
