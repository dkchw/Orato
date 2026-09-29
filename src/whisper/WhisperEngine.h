#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QList>
#include <vector>
#include <memory>
#include <atomic>
#include "../models/SessionData.h"

struct whisper_context;

class WhisperWorker : public QObject {
    Q_OBJECT

public:
    explicit WhisperWorker(QObject *parent = nullptr);
    ~WhisperWorker() override;

    void setModelPath(const QString &path);
    void setLanguage(const QString &lang);
    void setTranslate(bool translate);
    void setThreads(int threads);

public slots:
    void processAudio(const std::vector<float> &pcmSamples);
    void cancel();

signals:
    void started();
    void progress(int percent);
    void segmentReady(const AudioSegment &segment);
    void finished(const QList<AudioSegment> &segments);
    void error(const QString &message);

private:
    bool loadModelIfNeeded();

    QString m_modelPath;
    QString m_loadedModelPath;
    QString m_language = "de";
    bool m_translate = false;
    int m_threads = 4;

    whisper_context *m_ctx = nullptr;
    std::atomic<bool> m_abort{false};
};

class WhisperEngine : public QObject {
    Q_OBJECT

public:
    explicit WhisperEngine(QObject *parent = nullptr);
    ~WhisperEngine() override;

    void setModelPath(const QString &modelPath);
    QString modelPath() const { return m_modelPath; }

    void setLanguage(const QString &lang);
    QString language() const { return m_language; }

    void setTranslate(bool translate);
    bool translate() const { return m_translate; }

    void setThreads(int threads);
    int threads() const { return m_threads; }

    bool isRunning() const { return m_isRunning; }

public slots:
    void transcribe(const std::vector<float> &pcmSamples);
    void cancel();

signals:
    void transcriptionStarted();
    void progress(int percent);
    void segmentDiscovered(const AudioSegment &segment);
    void transcriptionCompleted(const QList<AudioSegment> &segments);
    void transcriptionFailed(const QString &errorMessage);

private:
    QThread m_workerThread;
    WhisperWorker *m_worker = nullptr;

    QString m_modelPath;
    QString m_language = "de";
    bool m_translate = false;
    int m_threads = 4;
    bool m_isRunning = false;
};
