#include "SessionManager.h"
#include "../audio/AudioUtils.h"
#include <QStandardPaths>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent) {
    QDir().mkpath(sessionsRootDirectory());
}

QString SessionManager::sessionsRootDirectory() const {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty() || !QDir(dataDir).exists()) {
        // Fallback to standard XDG data directory
        QString oratoDir = QDir::homePath() + "/.local/share/orato";
        QString recorderDir = QDir::homePath() + "/.local/share/recorder";
        if (QDir(oratoDir).exists()) {
            dataDir = oratoDir;
        } else if (QDir(recorderDir).exists()) {
            dataDir = recorderDir;
        } else {
            dataDir = oratoDir;
        }
    }
    return dataDir + "/sessions";
}

QString SessionManager::sessionDirectory(const QString &sessionId) const {
    return sessionsRootDirectory() + "/" + sessionId;
}

QString SessionManager::getAudioPath(const QString &sessionId) const {
    return sessionDirectory(sessionId) + "/audio.wav";
}

QString SessionManager::getNotePath(const QString &sessionId) const {
    return sessionDirectory(sessionId) + "/note.md";
}

QString SessionManager::getTtsDirectory(const QString &sessionId) const {
    return sessionDirectory(sessionId) + "/tts";
}

QList<SessionData> SessionManager::listSessions() const {
    QList<SessionData> list;
    QDir rootDir(sessionsRootDirectory());
    QStringList subDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);

    for (const auto &dirName : subDirs) {
        QString jsonPath = rootDir.absoluteFilePath(dirName + "/session.json");
        if (QFile::exists(jsonPath)) {
            QFile file(jsonPath);
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                if (doc.isObject()) {
                    SessionData data = SessionData::fromJson(doc.object());
                    list.append(data);
                }
            }
        }
    }
    return list;
}

SessionData SessionManager::createNewSession(const QString &title, const QString &language) {
    SessionData session;
    QDateTime now = QDateTime::currentDateTime();
    session.id = "session_" + now.toString("yyyyMMdd_hhmmss");
    session.title = title.isEmpty() ? tr("Training Session %1").arg(now.toString("yyyy-MM-dd hh:mm")) : title;
    session.createdAt = now;
    session.updatedAt = now;
    session.language = language;
    session.audioFileName = "audio.wav";
    session.noteMarkdown = QString(
        "# %1\n\n"
        "**Date:** %2  \n"
        "**Language:** %3  \n\n"
        "## Session Notes\n\n"
        "- Record audio using the Record button above.\n"
        "- Whisper.cpp will transcribe the sentences with timestamps.\n"
        "- Click any sentence to replay or compare with Pocket TTS!\n"
    ).arg(session.title, now.toString("yyyy-MM-dd hh:mm:ss"), language);

    QDir().mkpath(sessionDirectory(session.id));
    QDir().mkpath(getTtsDirectory(session.id));

    saveSession(session);
    emit sessionsListChanged();
    return session;
}

bool SessionManager::loadSession(const QString &sessionId, SessionData &outSession, std::vector<float> &outPcm) {
    QString dir = sessionDirectory(sessionId);
    QString jsonPath = dir + "/session.json";

    if (!QFile::exists(jsonPath)) {
        emit errorOccurred(tr("Session not found: %1").arg(sessionId));
        return false;
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("Failed to read session file: %1").arg(jsonPath));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid session data in %1").arg(jsonPath));
        return false;
    }

    outSession = SessionData::fromJson(doc.object());

    // Load note.md if exists
    QString notePath = getNotePath(sessionId);
    if (QFile::exists(notePath)) {
        QFile noteFile(notePath);
        if (noteFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&noteFile);
            outSession.noteMarkdown = in.readAll();
            noteFile.close();
        }
    }

    // Load audio.wav if exists
    QString audioPath = getAudioPath(sessionId);
    if (QFile::exists(audioPath)) {
        uint32_t dur = 0;
        AudioUtils::loadWavToMono16k(audioPath, outPcm, dur);
        outSession.durationMs = dur;
    } else {
        outPcm.clear();
    }

    emit sessionLoaded(sessionId);
    return true;
}

bool SessionManager::saveSession(SessionData &session, const std::vector<float> &pcmSamples) {
    if (session.id.isEmpty()) return false;

    QString dir = sessionDirectory(session.id);
    QDir().mkpath(dir);
    QDir().mkpath(getTtsDirectory(session.id));

    session.updatedAt = QDateTime::currentDateTime();

    // Save audio.wav if samples provided
    if (!pcmSamples.empty()) {
        QString audioPath = getAudioPath(session.id);
        AudioUtils::writeWavFile(audioPath, pcmSamples, 16000, 1);
        session.durationMs = static_cast<qint64>((pcmSamples.size() * 1000) / 16000);
        session.audioFileName = "audio.wav";
    }

    // Save note.md
    QString notePath = getNotePath(session.id);
    QFile noteFile(notePath);
    if (noteFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&noteFile);
        out << session.noteMarkdown;
        noteFile.close();
    }

    // Save session.json
    QString jsonPath = dir + "/session.json";
    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(session.toJson());
        jsonFile.write(doc.toJson(QJsonDocument::Indented));
        jsonFile.close();
    }

    emit sessionSaved(session.id);
    emit sessionsListChanged();
    return true;
}

bool SessionManager::deleteSession(const QString &sessionId) {
    QString dir = sessionDirectory(sessionId);
    QDir sessionDir(dir);
    if (sessionDir.exists()) {
        bool ok = sessionDir.removeRecursively();
        if (ok) {
            emit sessionDeleted(sessionId);
            emit sessionsListChanged();
            return true;
        }
    }
    return false;
}
