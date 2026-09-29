#pragma once

#include <QObject>
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QByteArray>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include <cstdint>

class AudioRecorder : public QObject {
    Q_OBJECT

public:
    enum class State {
        Stopped,
        Recording,
        Paused
    };
    Q_ENUM(State)

    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder() override;

    QList<QAudioDevice> availableDevices() const;
    QAudioDevice defaultDevice() const;

    State state() const { return m_state; }
    qint64 durationMs() const;
    const std::vector<float>& pcmSamples() const { return m_recordedPcm16k; }

public slots:
    bool startRecording(const QAudioDevice &device = QAudioDevice());
    void pauseRecording();
    void resumeRecording();
    void stopRecording(const QString &saveWavPath = QString());

signals:
    void stateChanged(AudioRecorder::State state);
    void durationChanged(qint64 ms);
    void levelChanged(float peak, float rms);
    void recordingFinished(const QString &savedPath, qint64 durationMs);
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();
    void onTimerTick();

private:
    void processRawBufferTo16k();

    State m_state = State::Stopped;
    QAudioDevice m_currentDevice;
    QAudioFormat m_audioFormat;
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_ioDevice = nullptr;

    QByteArray m_rawPcmBuffer;
    std::vector<float> m_recordedPcm16k;
    QString m_savePath;

    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
    qint64 m_accumulatedMs = 0;
};
