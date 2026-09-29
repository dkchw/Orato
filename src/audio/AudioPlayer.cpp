#include "AudioPlayer.h"
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent) {
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    m_audioOutput->setVolume(1.0f);

    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioPlayer::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioPlayer::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &AudioPlayer::onPlaybackStateChanged);
}

AudioPlayer::~AudioPlayer() {
    stop();
}

void AudioPlayer::setSource(const QString &filePath) {
    m_sourceFilePath = filePath;
    m_isSegmentActive = false;
    m_loopSegment = false;

    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        m_player->setSource(QUrl());
    } else {
        m_player->setSource(QUrl::fromLocalFile(filePath));
    }
}

qint64 AudioPlayer::duration() const {
    return m_player->duration();
}

qint64 AudioPlayer::position() const {
    return m_player->position();
}

bool AudioPlayer::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

bool AudioPlayer::isPaused() const {
    return m_player->playbackState() == QMediaPlayer::PausedState;
}

qreal AudioPlayer::playbackRate() const {
    return m_player->playbackRate();
}

float AudioPlayer::volume() const {
    return m_audioOutput->volume();
}

bool AudioPlayer::isMuted() const {
    return m_audioOutput->isMuted();
}

void AudioPlayer::play() {
    m_player->play();
}

void AudioPlayer::pause() {
    m_player->pause();
}

void AudioPlayer::togglePlayPause() {
    if (isPlaying()) {
        pause();
    } else {
        play();
    }
}

void AudioPlayer::stop() {
    m_isSegmentActive = false;
    m_player->stop();
}

void AudioPlayer::seek(qint64 positionMs) {
    m_player->setPosition(std::max<qint64>(0, std::min<qint64>(positionMs, duration())));
}

void AudioPlayer::skipBackward(qint64 ms) {
    seek(position() - ms);
}

void AudioPlayer::skipForward(qint64 ms) {
    seek(position() + ms);
}

void AudioPlayer::playSegment(qint64 startMs, qint64 endMs, bool loop) {
    if (startMs >= endMs) return;

    m_segmentStartMs = std::max<qint64>(0, startMs);
    m_segmentEndMs = std::min<qint64>(duration(), endMs);
    m_loopSegment = loop;
    m_isSegmentActive = true;

    seek(m_segmentStartMs);
    play();

    emit segmentStarted(m_segmentStartMs, m_segmentEndMs);
}

void AudioPlayer::stopSegmentPlayback() {
    m_isSegmentActive = false;
    m_loopSegment = false;
}

void AudioPlayer::setLoopSegment(bool loop) {
    m_loopSegment = loop;
}

void AudioPlayer::setPlaybackRate(qreal rate) {
    if (rate > 0.1 && rate <= 4.0) {
        m_player->setPlaybackRate(rate);
        emit rateChanged(rate);
    }
}

void AudioPlayer::setVolume(float volume) {
    float clamped = std::max(0.0f, std::min(1.0f, volume));
    m_audioOutput->setVolume(clamped);
    emit volumeChanged(clamped);
}

void AudioPlayer::setMuted(bool muted) {
    m_audioOutput->setMuted(muted);
}

void AudioPlayer::onPositionChanged(qint64 positionMs) {
    if (m_isSegmentActive) {
        if (positionMs >= m_segmentEndMs) {
            if (m_loopSegment) {
                seek(m_segmentStartMs);
                play();
            } else {
                pause();
                seek(m_segmentStartMs);
                m_isSegmentActive = false;
                emit segmentFinished(m_segmentStartMs, m_segmentEndMs);
            }
        }
    }
    emit positionChanged(positionMs);
}

void AudioPlayer::onDurationChanged(qint64 durationMs) {
    emit durationChanged(durationMs);
}

void AudioPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    emit playbackStateChanged(state);
}
