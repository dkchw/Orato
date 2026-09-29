#include "PocketTTSEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

PocketTTSEngine::PocketTTSEngine(QObject *parent)
    : QObject(parent) {
    detectEnvironment();

    m_voices.append({"juergen", "Jürgen (German)", "german"});
    m_voices.append({"alba", "Alba (English)", "english"});
    m_voices.append({"estelle", "Estelle (French)", "french"});
    m_voices.append({"lola", "Lola (Spanish)", "spanish"});
    m_voices.append({"giovanni", "Giovanni (Italian)", "italian"});
    m_voices.append({"rafael", "Rafael (Portuguese)", "portuguese"});
}

PocketTTSEngine::~PocketTTSEngine() {
    cancel();
}

void PocketTTSEngine::detectEnvironment() {
    QStringList searchDirs = {
        QDir::currentPath() + "/pocket-tts",
        QCoreApplication::applicationDirPath() + "/pocket-tts",
        QCoreApplication::applicationDirPath() + "/../pocket-tts",
        "/run/host/home/dkchw/Documents/Code/Ongoing/Repo/Recorder/pocket-tts"
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

    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &PocketTTSEngine::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &PocketTTSEngine::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PocketTTSEngine::onProcessFinished);

    QString program;
    QStringList args;

    QString lang = language.toLower();
    if (lang == "de") lang = "german";
    if (lang == "en") lang = "english";
    if (lang == "fr") lang = "french";
    if (lang == "es") lang = "spanish";
    if (lang == "it") lang = "italian";
    if (lang == "pt") lang = "portuguese";

    QString targetVoice = voice.isEmpty() ? defaultVoiceForLanguage(lang) : voice;

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

    emit generationStarted(text);
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
