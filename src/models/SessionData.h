#pragma once

#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

struct AudioSegment {
    int id = 0;
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;
    QString ttsAudioPath;

    double durationSec() const {
        return (endMs - startMs) / 1000.0;
    }

    QString formatTimeRange() const {
        auto formatMs = [](qint64 ms) -> QString {
            int totalSeconds = static_cast<int>(ms / 1000);
            int minutes = totalSeconds / 60;
            int seconds = totalSeconds % 60;
            int tenths = static_cast<int>((ms % 1000) / 100);
            return QString("%1:%2.%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(tenths);
        };
        return QString("%1 - %2").arg(formatMs(startMs), formatMs(endMs));
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["startMs"] = startMs;
        obj["endMs"] = endMs;
        obj["text"] = text;
        obj["ttsAudioPath"] = ttsAudioPath;
        return obj;
    }

    static AudioSegment fromJson(const QJsonObject &obj) {
        AudioSegment seg;
        seg.id = obj["id"].toInt();
        seg.startMs = obj["startMs"].toInteger();
        seg.endMs = obj["endMs"].toInteger();
        seg.text = obj["text"].toString();
        seg.ttsAudioPath = obj["ttsAudioPath"].toString();
        return seg;
    }
};

struct SessionTake {
    QString id;            // e.g. "take_1"
    QString name;          // e.g. "Take 1"
    QDateTime createdAt;
    qint64 durationMs = 0;
    QString audioFileName; // e.g. "take_1.wav"
    QList<AudioSegment> segments;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["createdAt"] = createdAt.toString(Qt::ISODate);
        obj["durationMs"] = durationMs;
        obj["audioFileName"] = audioFileName;
        QJsonArray segArray;
        for (const auto &seg : segments) {
            segArray.append(seg.toJson());
        }
        obj["segments"] = segArray;
        return obj;
    }

    static SessionTake fromJson(const QJsonObject &obj) {
        SessionTake take;
        take.id = obj["id"].toString();
        take.name = obj["name"].toString();
        take.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        take.durationMs = obj["durationMs"].toInteger();
        take.audioFileName = obj["audioFileName"].toString();
        QJsonArray segArray = obj["segments"].toArray();
        for (const auto &val : segArray) {
            take.segments.append(AudioSegment::fromJson(val.toObject()));
        }
        return take;
    }
};

struct SessionData {
    QString id;
    QString title;
    QDateTime createdAt;
    QDateTime updatedAt;
    qint64 durationMs = 0;
    QString audioFileName = "audio.wav"; // Relative to session directory
    QString noteMarkdown;
    QString whisperModel;
    QString language = "de";
    QList<AudioSegment> segments;
    QList<SessionTake> takes;
    int currentTakeIndex = 0;

    SessionTake* currentTake() {
        if (currentTakeIndex >= 0 && currentTakeIndex < takes.size()) {
            return &takes[currentTakeIndex];
        }
        return nullptr;
    }

    const SessionTake* currentTake() const {
        if (currentTakeIndex >= 0 && currentTakeIndex < takes.size()) {
            return &takes[currentTakeIndex];
        }
        return nullptr;
    }

    SessionTake& addNewTake(const QString &name = QString()) {
        SessionTake take;
        int nextNum = takes.size() + 1;
        take.id = QString("take_%1").arg(nextNum);
        take.name = name.isEmpty() ? QString("Take %1").arg(nextNum) : name;
        take.createdAt = QDateTime::currentDateTime();
        take.durationMs = 0;
        take.audioFileName = QString("take_%1.wav").arg(nextNum);
        takes.append(take);
        currentTakeIndex = takes.size() - 1;
        return takes.last();
    }

    bool removeTake(int index) {
        if (index < 0 || index >= takes.size() || takes.size() <= 1) {
            return false;
        }
        takes.removeAt(index);
        if (currentTakeIndex >= takes.size()) {
            currentTakeIndex = takes.size() - 1;
        }
        return true;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["title"] = title;
        obj["createdAt"] = createdAt.toString(Qt::ISODate);
        obj["updatedAt"] = updatedAt.toString(Qt::ISODate);
        obj["durationMs"] = durationMs;
        obj["audioFileName"] = audioFileName;
        obj["noteMarkdown"] = noteMarkdown;
        obj["whisperModel"] = whisperModel;
        obj["language"] = language;
        obj["currentTakeIndex"] = currentTakeIndex;

        QJsonArray segArray;
        for (const auto &seg : segments) {
            segArray.append(seg.toJson());
        }
        obj["segments"] = segArray;

        QJsonArray takesArray;
        for (const auto &t : takes) {
            takesArray.append(t.toJson());
        }
        obj["takes"] = takesArray;

        return obj;
    }

    static SessionData fromJson(const QJsonObject &obj) {
        SessionData data;
        data.id = obj["id"].toString();
        data.title = obj["title"].toString();
        data.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        data.updatedAt = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate);
        data.durationMs = obj["durationMs"].toInteger();
        data.audioFileName = obj["audioFileName"].toString("audio.wav");
        data.noteMarkdown = obj["noteMarkdown"].toString();
        data.whisperModel = obj["whisperModel"].toString();
        data.language = obj["language"].toString("de");
        data.currentTakeIndex = obj["currentTakeIndex"].toInt(0);

        QJsonArray segArray = obj["segments"].toArray();
        for (const auto &val : segArray) {
            data.segments.append(AudioSegment::fromJson(val.toObject()));
        }

        QJsonArray takesArray = obj["takes"].toArray();
        for (const auto &val : takesArray) {
            data.takes.append(SessionTake::fromJson(val.toObject()));
        }

        // Migration: If no takes exist yet, create Take 1 from session audio
        if (data.takes.isEmpty()) {
            SessionTake t1;
            t1.id = "take_1";
            t1.name = "Take 1";
            t1.createdAt = data.createdAt;
            t1.durationMs = data.durationMs;
            t1.audioFileName = data.audioFileName;
            t1.segments = data.segments;
            data.takes.append(t1);
            data.currentTakeIndex = 0;
        }

        return data;
    }
};
