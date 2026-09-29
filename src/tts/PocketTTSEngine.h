#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QMap>

struct VoiceInfo {
    QString id;
    QString displayName;
    QString language;
};

class PocketTTSEngine : public QObject {
    Q_OBJECT

public:
    explicit PocketTTSEngine(QObject *parent = nullptr);
    ~PocketTTSEngine() override;

    bool isAvailable() const;
    QString enginePath() const { return m_engineDir; }

    QList<VoiceInfo> availableVoices() const;
    QString defaultVoiceForLanguage(const QString &language) const;

public slots:
    void generateSpeech(const QString &text,
                        const QString &outputPath,
                        const QString &language = "german",
                        const QString &voice = "juergen");
    void cancel();

signals:
    void generationStarted(const QString &text);
    void generationProgress(const QString &log);
    void generationCompleted(const QString &outputPath);
    void generationFailed(const QString &errorMessage);

private slots:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void detectEnvironment();

    QString m_engineDir;
    QString m_executablePath;
    bool m_useUv = false;

    QProcess *m_process = nullptr;
    QString m_currentText;
    QString m_currentOutputPath;
    QList<VoiceInfo> m_voices;
};
