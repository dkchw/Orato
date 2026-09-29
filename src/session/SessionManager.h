#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QDir>
#include <vector>
#include "../models/SessionData.h"

class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager() override = default;

    QString sessionsRootDirectory() const;
    QString sessionDirectory(const QString &sessionId) const;

    QList<SessionData> listSessions() const;
    SessionData createNewSession(const QString &title = QString(), const QString &language = "de");
    bool loadSession(const QString &sessionId, SessionData &outSession, std::vector<float> &outPcm);
    bool saveSession(SessionData &session, const std::vector<float> &pcmSamples = {});
    bool deleteSession(const QString &sessionId);

    QString getAudioPath(const QString &sessionId) const;
    QString getNotePath(const QString &sessionId) const;
    QString getTtsDirectory(const QString &sessionId) const;
    QString getTakesDirectory(const QString &sessionId) const;
    QString getTakeAudioPath(const QString &sessionId, const QString &audioFileName) const;

signals:
    void sessionsListChanged();
    void sessionSaved(const QString &sessionId);
    void sessionLoaded(const QString &sessionId);
    void sessionDeleted(const QString &sessionId);
    void errorOccurred(const QString &message);
};
