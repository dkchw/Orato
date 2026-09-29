#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>

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

    void startServer(int port = 8989, const QString &language = "german", const QString &defaultVoice = "juergen");
    void stopServer();
    bool isServerReady() const { return m_serverReady; }

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
    void serverReady();

private slots:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onServerOutput();

private:
    void detectEnvironment();
    void generateViaHttp(const QString &text, const QString &outputPath, const QString &voice);
    void generateViaCli(const QString &text, const QString &outputPath, const QString &language, const QString &voice);

    QString m_engineDir;
    QString m_executablePath;
    bool m_useUv = false;

    QProcess *m_serverProcess = nullptr;
    int m_serverPort = 8989;
    bool m_serverReady = false;

    QProcess *m_process = nullptr;
    QNetworkAccessManager *m_netManager = nullptr;
    QNetworkReply *m_currentReply = nullptr;

    QString m_currentText;
    QString m_currentOutputPath;
    QList<VoiceInfo> m_voices;
};
