#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>

class AudioPlayer : public QObject {
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    void setSource(const QString &filePath);
    QString currentSource() const { return m_sourceFilePath; }

    qint64 duration() const;
    qint64 position() const;
    bool isPlaying() const;
    bool isPaused() const;

    qreal playbackRate() const;
    float volume() const;
    bool isMuted() const;

    bool isLoopingSegment() const { return m_loopSegment; }
    qint64 segmentStart() const { return m_segmentStartMs; }
    qint64 segmentEnd() const { return m_segmentEndMs; }

public slots:
    void play();
    void pause();
    void togglePlayPause();
    void stop();
    void seek(qint64 positionMs);
    void skipBackward(qint64 ms = 5000);
    void skipForward(qint64 ms = 5000);

    void playSegment(qint64 startMs, qint64 endMs, bool loop = false);
    void stopSegmentPlayback();
    void setLoopSegment(bool loop);

    void setPlaybackRate(qreal rate);
    void setVolume(float volume);
    void setMuted(bool muted);

signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void playbackStateChanged(QMediaPlayer::PlaybackState state);
    void rateChanged(qreal rate);
    void volumeChanged(float volume);
    void segmentStarted(qint64 startMs, qint64 endMs);
    void segmentFinished(qint64 startMs, qint64 endMs);

private slots:
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QString m_sourceFilePath;

    bool m_isSegmentActive = false;
    qint64 m_segmentStartMs = 0;
    qint64 m_segmentEndMs = 0;
    bool m_loopSegment = false;
};
