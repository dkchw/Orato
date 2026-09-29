#include "PocketTTSEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>

PocketTTSEngine::PocketTTSEngine(QObject *parent)
    : QObject(parent) {
    m_netManager = new QNetworkAccessManager(this);
    detectEnvironment();

    m_voices.append({"juergen", "Jürgen (German)", "german"});
    m_voices.append({"alba", "Alba (English)", "english"});
    m_voices.append({"estelle", "Estelle (French)", "french"});
    m_voices.append({"lola", "Lola (Spanish)", "spanish"});
    m_voices.append({"giovanni", "Giovanni (Italian)", "italian"});
    m_voices.append({"rafael", "Rafael (Portuguese)", "portuguese"});

    // Warm-up: Start persistent background server so synthesis is sub-second
    if (isAvailable()) {
        startServer(8989, "german", "juergen");
    }
}

PocketTTSEngine::~PocketTTSEngine() {
    stopServer();
    cancel();
}

void PocketTTSEngine::detectEnvironment() {
    QStringList searchDirs = {
        QDir::currentPath() + "/pocket-tts",
        QCoreApplication::applicationDirPath() + "/pocket-tts",
        QCoreApplication::applicationDirPath() + "/../pocket-tts",
        "/run/host/home/dkchw/Documents/Code/Ongoing/Repo/Orato/pocket-tts"
    };

    for (const auto &dir : searchDirs) {
        if (QDir(dir).exists()) {
            m_engineDir = dir;
            QString venvBin = dir + "/.venv/bin/pocket-tts";
            if (QFile::exists(venvBin)) {
                m_executablePath = venvBin;
                return;
            }
        }
    }

    // Check system PATH
    QString systemPocket = QStandardPaths::findExecutable("pocket-tts");
    if (!systemPocket.isEmpty()) {
        m_executablePath = systemPocket;
        return;
    }

    // Check uv
    QString uvPath = QStandardPaths::findExecutable("uv");
    if (!uvPath.isEmpty() && !m_engineDir.isEmpty()) {
        m_useUv = true;
        m_executablePath = uvPath;
        return;
    }
}

bool PocketTTSEngine::isAvailable() const {
    return !m_executablePath.isEmpty() || !m_engineDir.isEmpty();
}

QList<VoiceInfo> PocketTTSEngine::availableVoices() const {
    return m_voices;
}

QString PocketTTSEngine::defaultVoiceForLanguage(const QString &language) const {
    QString lang = language.toLower();
    if (lang == "german" || lang == "de") return "juergen";
    if (lang == "french" || lang == "fr") return "estelle";
    if (lang == "spanish" || lang == "es") return "lola";
    if (lang == "italian" || lang == "it") return "giovanni";
    if (lang == "portuguese" || lang == "pt") return "rafael";
    return "alba";
}

void PocketTTSEngine::startServer(int port, const QString &language, const QString &defaultVoice) {
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        return;
    }
    m_serverPort = port;
    m_serverProcess = new QProcess(this);
    connect(m_serverProcess, &QProcess::readyReadStandardOutput, this, &PocketTTSEngine::onServerOutput);
    connect(m_serverProcess, &QProcess::readyReadStandardError, this, &PocketTTSEngine::onServerOutput);

    QString program;
    QStringList args;
    if (m_useUv) {
        program = m_executablePath;
        args << "run" << "--directory" << m_engineDir << "pocket-tts" << "serve"
             << "--port" << QString::number(port)
             << "--language" << language
             << "--default-voice" << defaultVoice;
    } else {
        program = m_executablePath.isEmpty() ? "pocket-tts" : m_executablePath;
        args << "serve"
             << "--port" << QString::number(port)
             << "--language" << language
             << "--default-voice" << defaultVoice;
    }
    m_serverProcess->start(program, args);
}

void PocketTTSEngine::stopServer() {
    if (m_serverProcess) {
        m_serverProcess->kill();
        m_serverProcess->waitForFinished(1000);
        m_serverProcess->deleteLater();
        m_serverProcess = nullptr;
        m_serverReady = false;
    }
}

void PocketTTSEngine::onServerOutput() {
    if (!m_serverProcess) return;
    QString out = QString::fromUtf8(m_serverProcess->readAllStandardOutput() + m_serverProcess->readAllStandardError());
    if (out.contains("Application startup complete") || out.contains("Uvicorn running")) {
        m_serverReady = true;
        emit serverReady();
    }
}

void PocketTTSEngine::generateSpeech(const QString &text,
                                     const QString &outputPath,
                                     const QString &language,
                                     const QString &voice) {
    if (text.trimmed().isEmpty()) {
        emit generationFailed(tr("Text is empty."));
        return;
    }

    cancel();

    m_currentText = text;
    m_currentOutputPath = outputPath;

    // Ensure output directory exists
    QFileInfo outInfo(outputPath);
    QDir().mkpath(outInfo.absolutePath());

    QString lang = language.toLower();
    if (lang == "de") lang = "german";
    if (lang == "en") lang = "english";
    if (lang == "fr") lang = "french";
    if (lang == "es") lang = "spanish";
    if (lang == "it") lang = "italian";
    if (lang == "pt") lang = "portuguese";

    QString targetVoice = voice.isEmpty() ? defaultVoiceForLanguage(lang) : voice;

    emit generationStarted(text);

    if (m_serverReady) {
        generateViaHttp(text, outputPath, targetVoice);
    } else {
        generateViaCli(text, outputPath, lang, targetVoice);
    }
}

void PocketTTSEngine::generateViaHttp(const QString &text, const QString &outputPath, const QString &voice) {
    QUrl url(QString("http://127.0.0.1:%1/tts").arg(m_serverPort));
    QNetworkRequest request(url);

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    textPart.setBody(text.toUtf8());
    multiPart->append(textPart);

    if (!voice.isEmpty()) {
        QHttpPart voicePart;
        voicePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"voice_url\""));
        voicePart.setBody(voice.toUtf8());
        multiPart->append(voicePart);
    }

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    m_currentReply = m_netManager->post(request, multiPart);
    multiPart->setParent(m_currentReply);

    connect(m_currentReply, &QNetworkReply::finished, this, [this, outputPath]() {
        if (!m_currentReply) return;
        if (m_currentReply->error() == QNetworkReply::NoError) {
            QByteArray data = m_currentReply->readAll();
            QFile file(outputPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                emit generationCompleted(outputPath);
            } else {
                emit generationFailed(tr("Failed to save audio to %1").arg(outputPath));
            }
        } else {
            // Fall back to CLI if HTTP failed
            generateViaCli(m_currentText, outputPath, "german", "juergen");
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    });
}

void PocketTTSEngine::generateViaCli(const QString &text, const QString &outputPath, const QString &lang, const QString &targetVoice) {
    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &PocketTTSEngine::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &PocketTTSEngine::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PocketTTSEngine::onProcessFinished);

    QString program;
    QStringList args;

    if (m_useUv) {
        program = m_executablePath;
        args << "run" << "--directory" << m_engineDir << "pocket-tts"
             << "generate"
             << "--text" << text
             << "--language" << lang
             << "--voice" << targetVoice
             << "--output-path" << outputPath
             << "--quiet";
    } else {
        program = m_executablePath.isEmpty() ? "pocket-tts" : m_executablePath;
        args << "generate"
             << "--text" << text
             << "--language" << lang
             << "--voice" << targetVoice
             << "--output-path" << outputPath
             << "--quiet";
    }

    m_process->start(program, args);
}

void PocketTTSEngine::cancel() {
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void PocketTTSEngine::onProcessOutput() {
    if (!m_process) return;
    QString out = QString::fromUtf8(m_process->readAllStandardOutput());
    QString err = QString::fromUtf8(m_process->readAllStandardError());
    QString combined = (out + " " + err).trimmed();
    if (!combined.isEmpty()) {
        emit generationProgress(combined);
    }
}

void PocketTTSEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    bool ok = (exitCode == 0 && exitStatus == QProcess::NormalExit && QFile::exists(m_currentOutputPath));
    if (ok) {
        emit generationCompleted(m_currentOutputPath);
    } else {
        emit generationFailed(tr("TTS generation failed with exit code %1").arg(exitCode));
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}
