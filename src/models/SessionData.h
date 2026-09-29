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

struct SessionData {
    QString id;
    QString title;
    QDateTime createdAt;
    QDateTime updatedAt;
    qint64 durationMs = 0;
    QString audioFileName; // Relative to session directory, e.g. "audio.wav"
    QString noteMarkdown;
    QString whisperModel;
    QString language = "de";
    QList<AudioSegment> segments;

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

        QJsonArray segArray;
        for (const auto &seg : segments) {
            segArray.append(seg.toJson());
        }
        obj["segments"] = segArray;
        return obj;
    }

    static SessionData fromJson(const QJsonObject &obj) {
        SessionData data;
        data.id = obj["id"].toString();
        data.title = obj["title"].toString();
        data.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        data.updatedAt = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODate);
        data.durationMs = obj["durationMs"].toInteger();
        data.audioFileName = obj["audioFileName"].toString();
        data.noteMarkdown = obj["noteMarkdown"].toString();
        data.whisperModel = obj["whisperModel"].toString();
        data.language = obj["language"].toString("de");

        QJsonArray segArray = obj["segments"].toArray();
        for (const auto &val : segArray) {
            data.segments.append(AudioSegment::fromJson(val.toObject()));
        }
        return data;
    }
};
