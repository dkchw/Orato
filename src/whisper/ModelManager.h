#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QProcess>

struct ModelPreset {
    QString id;
    QString name;
    QString url;
    QString fileName;
    QString description;
    QString language; // "de", "en", "multilingual"
    qint64 approxSizeMb = 0;
};

struct InstalledModel {
    QString name;
    QString filePath;
    qint64 sizeBytes = 0;
    bool isCustom = false;
};

class ModelManager : public QObject {
    Q_OBJECT

public:
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager() override;

    QString modelsDirectory() const;
    QList<ModelPreset> presetModels() const;
    QList<InstalledModel> installedModels() const;

    bool isModelInstalled(const QString &presetId) const;
    QString getModelPath(const QString &presetId) const;

    bool removeModel(const QString &filePathOrName);
    bool deletePresetModel(const QString &presetId);

public slots:
    void refreshInstalledModels();
    void downloadPreset(const QString &presetId);
    void cancelDownload();
    void addCustomModel(const QString &filePath);
    void convertHuggingFaceModel(const QString &hfModelIdOrUrl);

signals:
    void modelsListChanged();
    void downloadStarted(const QString &modelName);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, int percent);
    void downloadCompleted(const QString &modelPath);
    void downloadFailed(const QString &errorMessage);

    void conversionProgress(const QString &logText);
    void conversionFinished(bool success, const QString &outputPath);

private slots:
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadReadyRead();
    void onDownloadFinished();

private:
    void initPresets();

    QList<ModelPreset> m_presets;
    QList<InstalledModel> m_installed;

    QNetworkAccessManager *m_netManager = nullptr;
    QNetworkReply *m_currentReply = nullptr;
    QFile *m_outputFile = nullptr;
    QString m_downloadingModelName;
    QString m_downloadingDestPath;

    QProcess *m_convertProcess = nullptr;
};
