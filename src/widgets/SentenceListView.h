#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include "../models/SessionData.h"

class SentenceItemWidget : public QFrame {
    Q_OBJECT

public:
    explicit SentenceItemWidget(const AudioSegment &segment, QWidget *parent = nullptr);

    const AudioSegment& segment() const { return m_segment; }
    void setActive(bool active);
    void updateTtsStatus(bool hasAudio);

signals:
    void playRequested(int id, qint64 startMs, qint64 endMs);
    void loopRequested(int id, qint64 startMs, qint64 endMs);
    void ttsRequested(int id, const QString &text);
    void sendToNoteRequested(int id, qint64 startMs, const QString &text);
    void textEdited(int id, const QString &newText);

private:
    AudioSegment m_segment;
    QLabel *m_badgeLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLineEdit *m_textEdit = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_loopBtn = nullptr;
    QPushButton *m_ttsBtn = nullptr;
    QPushButton *m_noteBtn = nullptr;
    bool m_isActive = false;
};

class SentenceListView : public QWidget {
    Q_OBJECT

public:
    explicit SentenceListView(QWidget *parent = nullptr);
    ~SentenceListView() override = default;

    void setSegments(const QList<AudioSegment> &segments);
    QList<AudioSegment> segments() const;
    void updatePlaybackPosition(qint64 ms);
    void markTtsAvailable(int id, const QString &audioPath);

signals:
    void playSentenceRequested(qint64 startMs, qint64 endMs, bool loop);
    void ttsSentenceRequested(int id, const QString &text);
    void appendToNoteRequested(qint64 startMs, const QString &text);
    void segmentsChanged();

private:
    void clearList();

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_containerWidget = nullptr;
    QVBoxLayout *m_containerLayout = nullptr;
    QList<SentenceItemWidget*> m_itemWidgets;
    int m_activeSegmentId = -1;
};
